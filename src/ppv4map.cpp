#include "ppv4map.hpp"

#include <cmath>
#include <mutex>

#include "maps.hpp"

using namespace MetaCore;

namespace {
    DECLARE_JSON_STRUCT(Response) {
        NAMED_VALUE(UnparsedJSON, Difficulty, "difficulty");
    };
    DECLARE_JSON_STRUCT(Extra) {
        NAMED_VALUE(UnparsedJSON, Data, "ppv4");
    };

    using Key = std::pair<std::string, std::vector<std::string>>;
    CacheMap<Key, std::optional<PPv4::Context>, 32> contexts;
    std::mutex contextsMutex;

    // Keep the original precision unless a caller has edited the legacy ratings.
    void Override(double& value, float legacy) {
        if (static_cast<float>(value) != legacy)
            value = legacy;
    }

    void ApplyLegacyRatings(PPv4::Difficulty& difficulty, PP::BLSongDiff const& map) {
        difficulty.status = map.RankedStatus;
        Override(difficulty.base.acc, map.Acc);
        Override(difficulty.base.pass, map.Pass);
        Override(difficulty.base.tech, map.Tech);
        Override(difficulty.base.predicted, map.Predicted);
        if (difficulty.base.reportedStars)
            Override(*difficulty.base.reportedStars, map.Stars);
        std::map<std::string, double> modifiers;
        for (auto const& [key, value] : map.ModifierValues) {
            auto original = difficulty.modifierValues.find(key);
            modifiers[key] = original != difficulty.modifierValues.end() && static_cast<float>(original->second) == value ? original->second : value;
        }
        difficulty.modifierValues = std::move(modifiers);
        if (!map.ModifierRatings) {
            difficulty.hasSpeedRatings = false;
            difficulty.speeds.clear();
            return;
        }
        auto apply = [&](std::string const& key, float acc, float pass, float tech) {
            if (auto it = difficulty.speeds.find(key); it != difficulty.speeds.end()) {
                Override(it->second.acc, acc);
                Override(it->second.pass, pass);
                Override(it->second.tech, tech);
            }
        };
        auto const& speeds = *map.ModifierRatings;
        apply("ss", speeds.ssAccRating, speeds.ssPassRating, speeds.ssTechRating);
        apply("fs", speeds.fsAccRating, speeds.fsPassRating, speeds.fsTechRating);
        apply("sf", speeds.sfAccRating, speeds.sfPassRating, speeds.sfTechRating);
    }
}

std::optional<PP::BLSongDiff> PPv4::ParseMap(std::string const& json) {
    auto difficulty = ParseDifficulty(json);
    if (!difficulty)
        return std::nullopt;
    try {
        PP::BLSongDiff result = {};
        result.Difficulty = difficulty->name;
        result.Characteristic = difficulty->mode;
        result.RankedStatus = difficulty->status;
        result.Stars = difficulty->base.reportedStars.value_or(0);
        result.Pass = difficulty->base.pass;
        result.Acc = difficulty->base.acc;
        result.Tech = difficulty->base.tech;
        result.Predicted = difficulty->base.predicted;
        for (auto const& [key, value] : difficulty->modifierValues)
            result.ModifierValues[key] = value;
        if (difficulty->hasSpeedRatings) {
            PP::BLSpeedModifiers speeds = {};
            auto copy = [&](std::string const& key, float& acc, float& pass, float& tech) {
                if (auto it = difficulty->speeds.find(key); it != difficulty->speeds.end()) {
                    acc = it->second.acc;
                    pass = it->second.pass;
                    tech = it->second.tech;
                }
            };
            copy("ss", speeds.ssAccRating, speeds.ssPassRating, speeds.ssTechRating);
            copy("fs", speeds.fsAccRating, speeds.fsPassRating, speeds.fsTechRating);
            copy("sf", speeds.sfAccRating, speeds.sfPassRating, speeds.sfTechRating);
            result.ModifierRatings = speeds;
        }
        // extraFields already exists in the released BLSongDiff layout and survives copies by older mods.
        Extra extra;
        extra.Data = ReadFromString<Response>(json).Difficulty;
        result.extraFields = UnparsedJSON(extra).GetValue();
        return result;
    } catch (JSONException const&) {
        return std::nullopt;
    }
}

std::optional<PPv4::Context> PPv4::Prepare(PP::BLSongDiff const& map, std::vector<std::string> const& modifiers) {
    if (!map.extraFields)
        return std::nullopt;
    for (float value : {map.Acc, map.Pass, map.Tech, map.Predicted, map.Stars})
        if (!std::isfinite(value))
            return std::nullopt;
    for (auto const& [key, value] : map.ModifierValues)
        if (!std::isfinite(value))
            return std::nullopt;
    if (map.ModifierRatings) {
        auto const& ratings = *map.ModifierRatings;
        for (float value :
             {ratings.ssAccRating,
              ratings.ssPassRating,
              ratings.ssTechRating,
              ratings.fsAccRating,
              ratings.fsPassRating,
              ratings.fsTechRating,
              ratings.sfAccRating,
              ratings.sfPassRating,
              ratings.sfTechRating})
            if (!std::isfinite(value))
                return std::nullopt;
    }
    Key key{WriteToString(map), modifiers};
    {
        std::lock_guard lock(contextsMutex);
        if (contexts.contains(key))
            return contexts[key];
    }
    std::optional<Context> result;
    try {
        auto extra = UnparsedJSON::FromValue(map.extraFields->document).Parse<Extra>();
        if (auto difficulty = ParseDifficulty(WriteToString(extra.Data), false)) {
            ApplyLegacyRatings(*difficulty, map);
            if (difficulty->Eligible())
                result = Prepare(*difficulty, modifiers);
        }
    } catch (JSONException const&) {
        return std::nullopt;
    }
    std::lock_guard lock(contextsMutex);
    contexts.push(std::move(key), result);
    return result;
}

void PPv4::ClearCache() {
    std::lock_guard lock(contextsMutex);
    contexts.clear();
}
