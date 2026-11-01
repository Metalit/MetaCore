#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "legacy-pp.hpp"
#include "ppv4map.hpp"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace MetaCore::PPv4;
static_assert(sizeof(MetaCore::PP::BLSongDiff) == sizeof(LegacyPP::BLSongDiff));
static_assert(alignof(MetaCore::PP::BLSongDiff) == alignof(LegacyPP::BLSongDiff));
static_assert(sizeof(MetaCore::PP::BLSpeedModifiers) == sizeof(LegacyPP::BLSpeedModifiers));
template <class T>
auto Offsets(T const& value) {
    auto offset = [&](auto const& field) {
        return reinterpret_cast<char const*>(&field) - reinterpret_cast<char const*>(&value);
    };
    return std::vector<std::ptrdiff_t>{
        offset(value.extraFields),
        offset(value.Difficulty),
        offset(value.Characteristic),
        offset(value.RankedStatus),
        offset(value.Stars),
        offset(value.Predicted),
        offset(value.Pass),
        offset(value.Acc),
        offset(value.Tech),
        offset(value.ModifierValues),
        offset(value.ModifierRatings)
    };
}
std::string Serialize(rapidjson::Value const& value) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);
    value.Accept(w);
    return s.GetString();
}
void Check(bool test, std::string const& description) {
    if (!test)
        throw std::runtime_error(description);
}
std::vector<std::string> Mods(std::string s) {
    for (auto& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    std::stringstream in(s);
    std::vector<std::string> result;
    std::string code;
    while (std::getline(in, code, ','))
        if (!code.empty())
            result.push_back(code);
    return result;
}
int main(int argc, char** argv) try {
    Check(Offsets(MetaCore::PP::BLSongDiff{}) == Offsets(LegacyPP::BLSongDiff{}), "Released field layout changed");
    Check(argc == 2, "Usage: vectors fixtures.json");
    std::ifstream input(argv[1]);
    Check(input.good(), "Fixture missing");
    std::string json((std::istreambuf_iterator<char>(input)), {});
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    Check(!doc.HasParseError(), "Fixture JSON invalid");
    std::map<std::string, Difficulty> maps;
    std::map<std::string, MetaCore::PP::BLSongDiff> legacyMaps;
    double maxStars = 0, maxPp = 0;
    int stars = 0, vectors = 0;
    for (auto const& j : doc["beatleader"]["leaderboards"].GetArray()) {
        auto parsed = ParseDifficulty(Serialize(j), false);
        Check(parsed.has_value(), "Map parse failed");
        auto d = *parsed;
        maps.emplace(Serialize(j["leaderboardId"]), d);
        auto legacy = ParseMap("{\"difficulty\":" + Serialize(j) + "}");
        Check(legacy.has_value(), "Legacy map bridge failed");
        legacyMaps.emplace(Serialize(j["leaderboardId"]), *legacy);
        auto c = Prepare(d, {}).value();
        maxStars = std::max(maxStars, std::abs(c.stars - *d.base.reportedStars));
        ++stars;
        for (auto code : {"ss", "fs", "sf"}) {
            c = Prepare(d, {code}).value();
            maxStars = std::max(maxStars, std::abs(c.stars - *d.speeds.at(code).reportedStars));
            ++stars;
        }
    }
    for (auto const& j : doc["beatleader"]["vectors"].GetArray()) {
        auto d = maps.at(Serialize(j["leaderboardId"]));
        auto mods = Mods(j["modifiers"].GetString());
        auto c = Prepare(d, mods).value();
        auto r = c.Evaluate(j["accuracy"].GetDouble());
        auto bridge = Prepare(legacyMaps.at(Serialize(j["leaderboardId"])), mods);
        Check(bridge.has_value(), "Legacy context unavailable");
        Check(std::abs(bridge->Evaluate(j["accuracy"].GetDouble()).total - r.total) < 1e-8, "Legacy bridge changed PP");
        auto const& e = j["expected"];
        for (auto [actual, key] : {std::pair{r.total, "pp"}, {r.pass, "passPP"}, {r.acc, "accPP"}, {r.tech, "techPP"}}) {
            double error = std::abs(actual - e[key].GetDouble());
            maxPp = std::max(maxPp, error);
            Check(error <= .01, "PP vector mismatch: " + Serialize(j["scoreId"]) + " " + key);
        }
        double fcError = std::abs(c.Evaluate(j["fcAccuracy"].GetDouble()).total - j["expectedFcPp"].GetDouble());
        maxPp = std::max(maxPp, fcError);
        Check(fcError <= .01, "FC PP mismatch");
        ++vectors;
    }
    Check(maxStars < .0001, "Stars mismatch");
    auto d = maps.begin()->second;
    auto c = Prepare(d, {}).value();
    for (double x : {0., -1., 1.01, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()})
        Check(c.Evaluate(x).total == 0, "Invalid accuracy");
    Check(Prepare(d, {"nf"})->Evaluate(.95).total == 0, "No Fail must be zero");
    Check(Prepare(d, {"gn", "gn"})->Evaluate(.95).total == Prepare(d, {"gn"})->Evaluate(.95).total, "Duplicate mods");
    auto fs = Prepare(d, {"fs"}).value();
    d.modifierValues["fs"] = 1000;
    Check(fs.Evaluate(.95).total == Prepare(d, {"fs"})->Evaluate(.95).total, "Speed double applied");
    d.noteSpan.reset();
    Check(!Prepare(d, {}), "Missing v4 data must not silently calculate");
    for (int status : {0, 1, 4, 5, 7}) {
        d.status = status;
        Check(!d.Eligible(), "Unranked status");
    }
    for (int status : {2, 3, 6}) {
        d.status = status;
        Check(d.Eligible(), "Eligible status");
    }
    Check(EncodeSegment("90Degree /a") == "90Degree%20%2Fa", "Path encoding");
    for (std::string bad : {"null", "[]", "{}", "{\"difficulty\":null}", "{bad"}) {
        Check(!ParseDifficulty(bad), "Malformed response accepted");
    }
    auto unranked = ParseDifficulty(R"({"difficulty":{"difficultyName":"ExpertPlus","modeName":"Standard","status":0,"stars":99}})");
    Check(unranked && !unranked->Eligible(), "Unranked positive stars");
    auto legacy = legacyMaps.begin()->second;
    auto original = Prepare(legacy, {}).value().Evaluate(.95).total;
    auto copied = legacy;
    Check(Prepare(copied, {})->Evaluate(.95).total == original, "Copy lost PPv4 metadata");
    auto restored = ReadFromString<MetaCore::PP::BLSongDiff>(WriteToString(legacy));
    Check(Prepare(restored, {})->Evaluate(.95).total == original, "JSON round trip lost PPv4 metadata");
    legacy.Acc *= .5f;
    Check(Prepare(legacy, {})->Evaluate(.95).total < original, "Changed legacy ratings were ignored");
    legacy.Acc = std::numeric_limits<float>::quiet_NaN();
    Check(!Prepare(legacy, {}), "Invalid legacy rating accepted");
    legacy = copied;
    legacy.extraFields.reset();
    Check(!Prepare(legacy, {}), "Missing metadata must return no calculation");
    ClearCache();
    Check(Prepare(copied, {})->Evaluate(.95).total == original, "Cache clear changed PP");

    auto snapshot = Serialize(doc["beatleader"]["leaderboards"][0]);
    rapidjson::Document edited;
    edited.Parse(snapshot.c_str());
    edited["passRating"].SetNull();
    auto nullable = ParseDifficulty(Serialize(edited), false);
    Check(nullable && nullable->base.pass == 0, "Nullable rating should be zero");
    edited["passRating"].SetString("bad");
    Check(!ParseDifficulty(Serialize(edited), false), "Invalid rating accepted");
    edited.Parse(snapshot.c_str());
    edited.RemoveMember("noteSpan");
    auto missing = ParseDifficulty(Serialize(edited), false);
    Check(missing && !Prepare(*missing, {}), "Missing noteSpan accepted for calculation");
    edited.Parse(snapshot.c_str());
    edited["modifiersRating"].SetString("bad");
    Check(!ParseDifficulty(Serialize(edited), false), "Invalid speed ratings accepted");
    edited.Parse(snapshot.c_str());
    edited["status"].SetString("Ranked");
    Check(ParseDifficulty(Serialize(edited), false)->status == 3, "Named ranked status rejected");

    std::cout << "PASS: " << vectors << " BeatLeader vectors + " << stars << " star values + edge cases. Max PP error " << maxPp
              << ", max star error " << maxStars << "\n";
    return 0;
} catch (std::exception const& e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
}
