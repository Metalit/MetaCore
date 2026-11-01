#pragma once
#include "rapidjson-macros/shared/macros.hpp"

// Released 1.6.3 layouts; changes here require recompiling existing consumers.
namespace LegacyPP {
    /// @brief The BeatLeader ratings for speed-changing modifiers specifically
    DECLARE_JSON_STRUCT(BLSpeedModifiers) {
        VALUE(float, ssPassRating);
        VALUE(float, ssAccRating);
        VALUE(float, ssTechRating);
        VALUE(float, fsPassRating);
        VALUE(float, fsAccRating);
        VALUE(float, fsTechRating);
        VALUE(float, sfPassRating);
        VALUE(float, sfAccRating);
        VALUE(float, sfTechRating);
    };

    /// @brief BeatLeader ranking information for a specific characteristic/difficulty of a map
    DECLARE_JSON_STRUCT(BLSongDiff) {
        NAMED_VALUE(std::string, Difficulty, "difficultyName");
        NAMED_VALUE(std::string, Characteristic, "modeName");
        NAMED_VALUE_DEFAULT(int, RankedStatus, 0, "status");
        NAMED_VALUE_DEFAULT(float, Stars, 0, "stars");
        NAMED_VALUE_DEFAULT(float, Predicted, 0, "predictedAcc");
        NAMED_VALUE_DEFAULT(float, Pass, 0, "passRating");
        NAMED_VALUE_DEFAULT(float, Acc, 0, "accRating");
        NAMED_VALUE_DEFAULT(float, Tech, 0, "techRating");
        NAMED_MAP_DEFAULT(float, ModifierValues, {}, "modifierValues");
        NAMED_VALUE_OPTIONAL(BLSpeedModifiers, ModifierRatings, "modifiersRating");
    };

}
