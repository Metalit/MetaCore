#include <algorithm>
#include <cctype>
#include <cmath>

#include "ppv4.hpp"
#include "rapidjson-macros/shared/macros.hpp"

using namespace MetaCore::PPv4;

namespace {
    using Number = TypeOptions<double, UnparsedJSON>;
    using Status = TypeOptions<int, std::string>;

    DECLARE_JSON_STRUCT(Leaderboard) {
        NAMED_VALUE(UnparsedJSON, Difficulty, "difficulty");
    };

    DECLARE_JSON_STRUCT(Header) {
        KEEP_EXTRA_FIELDS;
        NAMED_VALUE(std::string, Name, "difficultyName");
        NAMED_VALUE(std::string, Mode, "modeName");
        NAMED_VALUE(Status, RankedStatus, "status");
    };

    DECLARE_JSON_STRUCT(RatingData) {
        NAMED_VALUE(Number, Acc, "accRating");
        NAMED_VALUE(Number, Pass, "passRating");
        NAMED_VALUE(Number, Tech, "techRating");
        NAMED_VALUE(Number, Predicted, "predictedAcc");
        NAMED_VALUE(Number, Sliders, "sliderPercentage");
        NAMED_VALUE_OPTIONAL(Number, Stars, "stars");
        NAMED_VALUE(int, Notes, "notes");
        NAMED_VALUE(int, MaxScore, "maxScore");
        NAMED_VALUE_OPTIONAL(Number, NoteSpan, "noteSpan");
        NAMED_VALUE_OPTIONAL(UnparsedJSON, Modifiers, "modifierValues");
        NAMED_VALUE_OPTIONAL(UnparsedJSON, Speeds, "modifiersRating");
    };

    DECLARE_JSON_STRUCT(SpeedData) {
        VALUE_OPTIONAL(Number, ssAccRating);
        VALUE_OPTIONAL(Number, ssPassRating);
        VALUE_OPTIONAL(Number, ssTechRating);
        VALUE_OPTIONAL(Number, ssPredictedAcc);
        VALUE_OPTIONAL(Number, ssSliderPercentage);
        VALUE_OPTIONAL(Number, ssStars);
        VALUE_OPTIONAL(Number, fsAccRating);
        VALUE_OPTIONAL(Number, fsPassRating);
        VALUE_OPTIONAL(Number, fsTechRating);
        VALUE_OPTIONAL(Number, fsPredictedAcc);
        VALUE_OPTIONAL(Number, fsSliderPercentage);
        VALUE_OPTIONAL(Number, fsStars);
        VALUE_OPTIONAL(Number, sfAccRating);
        VALUE_OPTIONAL(Number, sfPassRating);
        VALUE_OPTIONAL(Number, sfTechRating);
        VALUE_OPTIONAL(Number, sfPredictedAcc);
        VALUE_OPTIONAL(Number, sfSliderPercentage);
        VALUE_OPTIONAL(Number, sfStars);
    };

    DECLARE_JSON_STRUCT(ModifierData) {
        NAMED_MAP(Number, Values, SELF_OBJECT_NAME);
    };

    bool IsNull(Number const& number) {
        auto json = number.GetValue<UnparsedJSON>();
        return json && json->GetValue().IsNull();
    }

    std::optional<double> Value(Number const& number) {
        if (auto value = number.GetValue<double>(); value && std::isfinite(*value))
            return value;
        if (IsNull(number))
            return 0;
        return std::nullopt;
    }

    std::optional<Ratings> ReadRatings(
        Number const& acc, Number const& pass, Number const& tech, Number const& predicted, Number const& sliders, std::optional<Number> const& stars
    ) {
        auto a = Value(acc), p = Value(pass), t = Value(tech), prediction = Value(predicted), slider = Value(sliders);
        if (!a || !p || !t || !prediction || !slider || *a < 0 || *p < 0 || *t < 0)
            return std::nullopt;
        Ratings result{*a, *p, *t, *prediction, *slider};
        if (stars && !IsNull(*stars)) {
            result.reportedStars = Value(*stars);
            if (!result.reportedStars)
                return std::nullopt;
        }
        return result;
    }

    std::optional<int> ReadStatus(Status const& status) {
        if (auto number = status.GetValue<int>(); number && *number >= 0 && *number <= 7)
            return number;
        auto name = status.GetValue<std::string>();
        if (!name)
            return std::nullopt;
        std::transform(name->begin(), name->end(), name->begin(), [](unsigned char c) { return std::tolower(c); });
        std::map<std::string, int> const names = {
            {"unranked", 0}, {"nominated", 1}, {"qualified", 2}, {"ranked", 3}, {"unrankable", 4}, {"outdated", 5}, {"inevent", 6}, {"ost", 7}
        };
        if (auto it = names.find(*name); it != names.end())
            return it->second;
        if (name->size() == 1 && (*name)[0] >= '0' && (*name)[0] <= '7')
            return (*name)[0] - '0';
        return std::nullopt;
    }
}

std::optional<Difficulty> MetaCore::PPv4::ParseDifficulty(std::string const& json, bool wrapped) {
    try {
        auto header = wrapped ? ReadFromString<Leaderboard>(json).Difficulty.Parse<Header>() : ReadFromString<Header>(json);
        auto status = ReadStatus(header.RankedStatus);
        if (!status)
            return std::nullopt;
        Difficulty result;
        result.name = header.Name;
        result.mode = header.Mode;
        result.status = *status;
        if (!result.Eligible())
            return result;
        auto data = UnparsedJSON::FromValue(header.extraFields->document).Parse<RatingData>();
        auto ratings = ReadRatings(data.Acc, data.Pass, data.Tech, data.Predicted, data.Sliders, data.Stars);
        if (!ratings || data.Notes < 0 || data.MaxScore <= 0)
            return std::nullopt;
        result.base = *ratings;
        result.notes = data.Notes;
        result.maxScore = data.MaxScore;
        if (data.NoteSpan && !IsNull(*data.NoteSpan)) {
            result.noteSpan = Value(*data.NoteSpan);
            if (!result.noteSpan || *result.noteSpan < 0)
                return std::nullopt;
        }
        if (data.Modifiers && !data.Modifiers->GetValue().IsNull()) {
            for (auto const& [key, number] : data.Modifiers->Parse<ModifierData>().Values) {
                if (key == "modifierId")
                    continue;
                auto value = Value(number);
                if (!value)
                    return std::nullopt;
                result.modifierValues[key] = *value;
            }
        }
        if (data.Speeds && !data.Speeds->GetValue().IsNull()) {
            auto speeds = data.Speeds->Parse<SpeedData>();
            result.hasSpeedRatings = true;
            auto add = [&](std::string key,
                           auto const& acc,
                           auto const& pass,
                           auto const& tech,
                           auto const& predicted,
                           auto const& sliders,
                           auto const& stars) {
                if (!acc || IsNull(*acc))
                    return true;
                if (!pass || !tech || !predicted || !sliders)
                    return false;
                auto rating = ReadRatings(*acc, *pass, *tech, *predicted, *sliders, stars);
                if (!rating)
                    return false;
                result.speeds.emplace(key, *rating);
                return true;
            };
            if (!add(
                    "ss", speeds.ssAccRating, speeds.ssPassRating, speeds.ssTechRating, speeds.ssPredictedAcc, speeds.ssSliderPercentage, speeds.ssStars
                ) ||
                !add(
                    "fs", speeds.fsAccRating, speeds.fsPassRating, speeds.fsTechRating, speeds.fsPredictedAcc, speeds.fsSliderPercentage, speeds.fsStars
                ) ||
                !add(
                    "sf", speeds.sfAccRating, speeds.sfPassRating, speeds.sfTechRating, speeds.sfPredictedAcc, speeds.sfSliderPercentage, speeds.sfStars
                ))
                return std::nullopt;
        }
        return result;
    } catch (JSONException const&) {
        return std::nullopt;
    }
}
