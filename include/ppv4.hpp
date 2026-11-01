#pragma once

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace MetaCore::PPv4 {
    using Curve = std::vector<std::pair<double, double>>;
    struct Ratings {
        double acc = 0, pass = 0, tech = 0, predicted = 0, sliders = 0;
        std::optional<double> reportedStars;
    };
    struct Difficulty {
        int status = 0, notes = 0, maxScore = 0;
        std::string name, mode;
        Ratings base;
        std::optional<double> noteSpan;
        std::map<std::string, Ratings> speeds;
        std::map<std::string, double> modifierValues;
        bool hasSpeedRatings = false;
        bool Eligible() const { return status == 2 || status == 3 || status == 6; }
    };
    struct Result {
        double total = 0, pass = 0, acc = 0, tech = 0;
    };
    struct Context {
        Curve curve;
        double stars = 0, power = 1, bonus = 1, curve96 = 0;
        double pass = 0, accWeight = 0, techWeight = 0;
        bool noFail = false, approximate = false;
        Result Evaluate(double accuracy) const;
    };
    Curve BuildCurve(double predicted, double sliders, int notes, double span);
    double Interpolate(Curve const& curve, double accuracy);
    double Stars(Ratings const& ratings, Curve const& curve);
    std::optional<Context> Prepare(Difficulty const& difficulty, std::vector<std::string> const& modifiers);
    // Returns no value for malformed or missing data. Nullable numeric values are zero.
    std::optional<Difficulty> ParseDifficulty(std::string const& json, bool wrapped = true);
    std::string EncodeSegment(std::string const& value);
}  // namespace MetaCore::PPv4
