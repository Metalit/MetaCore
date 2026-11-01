#include "ppv4.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <set>

namespace MetaCore::PPv4 {
    namespace {
        // BeatLeader PPv4 curve parameters (2026-10-30).
        Curve const base = {{1, 7.424},     {.999, 6.241}, {.9975, 5.158}, {.995, 4.010}, {.9925, 3.241}, {.99, 2.700}, {.9875, 2.303}, {.985, 2.007},
                            {.9825, 1.786}, {.98, 1.618},  {.9775, 1.490}, {.975, 1.392}, {.9725, 1.315}, {.97, 1.256}, {.965, 1.167},  {.96, 1.094},
                            {.955, 1.039},  {.95, 1},      {.94, .931},    {.93, .867},   {.92, .813},    {.91, .781},  {.9, .753},     {.875, .699},
                            {.85, .649},    {.825, .605},  {.8, .568},     {.75, .485},   {.7, .414},     {.65, .355},  {.6, .307},     {0, 0}};
        std::array<std::array<double, 7>, 9> const rows = {
            {{.955, 1, -5.30, 4.86, .60, 2.88, 2.46},
             {.960, .99, -3, 4.30, .66, 4.80, 1.85},
             {.965, .988, -2.4, 4.50, .64, 4.50, 1.95},
             {.970, .986, -2.2, 4.70, .62, 4.20, 2.05},
             {.975, .985, -1.8, 4.86, .60, 3.80, 2.15},
             {.980, .985, -1.4, 4.86, .60, 3.35, 2.30},
             {.985, .985, -1.2, 4.86, .60, 2.88, 2.46},
             {.990, .9915, -1.2, 4.86, .60, 2.88, 2.46},
             {.995, .9915, -1.2, 4.86, .60, 2.88, 2.46}}
        };
        struct Shape {
            double a = 1, c = 1, k = 4.86, w = .6, k2 = 2.88, p2 = 2.46;
            double M() const { return .744 / .35 * ((1 - w) + w * k * std::exp(k) / std::expm1(k)); }
            double F2(double x) const {
                return 1 + M() * (x - .95) + (6.424 - .05 * M()) * std::expm1(k2 * std::pow((x - .95) / .05, p2)) / std::expm1(k2);
            }
            double Value(double x) const {
                if (x <= .6)
                    return .256 / .6 * x;
                if (x <= .95) {
                    double u = (x - .6) / .35;
                    return .256 + .744 * ((1 - w) * u + w * std::expm1(k * u) / std::expm1(k));
                }
                if (x <= a)
                    return F2(x);
                double u = (a - .95) / .05;
                double slope = M() + (6.424 - .05 * M()) * std::exp(k2 * std::pow(u, p2)) / std::expm1(k2) * (k2 * p2 / .05) * std::pow(u, p2 - 1);
                double tail = slope * (1 - c);
                double result = F2(a) + tail * (x - a);
                if (std::abs(a - 1) >= 1e-9)
                    result += (slope - tail) * (1 - a) * (-std::expm1(-(x - a) / (1 - a)));
                return result;
            }
        };
        Shape Parameters(double predicted) {
            auto r = rows.front();
            if (predicted > rows.back()[0])
                r = rows.back();
            else if (predicted > rows.front()[0]) {
                size_t i = 1;
                while (predicted > rows[i][0])
                    ++i;
                double t = (predicted - rows[i - 1][0]) / (rows[i][0] - rows[i - 1][0]);
                for (size_t j = 1; j < 7; ++j)
                    r[j] = rows[i - 1][j] + t * (rows[i][j] - rows[i - 1][j]);
            }
            return {std::clamp(r[1], .951, 1.), std::min(r[2], 1.), r[3], r[4], r[5], r[6]};
        }
        struct Smooth {
            Curve points{base.rbegin(), base.rend()};
            std::array<double, 32> slopes{};
            Smooth() {
                std::array<double, 31> h{}, d{};
                for (size_t i = 0; i < 31; ++i) {
                    h[i] = points[i + 1].first - points[i].first;
                    d[i] = (points[i + 1].second - points[i].second) / h[i];
                }
                slopes[0] = d[0];
                slopes[31] = d[30];
                for (size_t i = 1; i < 31; ++i)
                    if (d[i - 1] * d[i] > 0) {
                        double w1 = 2 * h[i] + h[i - 1], w2 = h[i] + 2 * h[i - 1];
                        slopes[i] = (w1 + w2) / (w1 / d[i - 1] + w2 / d[i]);
                    }
            }
            double Value(double x) const {
                if (x <= 0)
                    return 0;
                if (x >= 1)
                    return points.back().second;
                size_t i = 0;
                while (points[i + 1].first < x)
                    ++i;
                double h = points[i + 1].first - points[i].first, t = (x - points[i].first) / h, t2 = t * t, t3 = t2 * t;
                return (2 * t3 - 3 * t2 + 1) * points[i].second + (t3 - 2 * t2 + t) * h * slopes[i] + (-2 * t3 + 3 * t2) * points[i + 1].second +
                       (t3 - t2) * h * slopes[i + 1];
            }
        };
        double Pass(double p) {
            if (p < 0)
                return 0;
            if (p > 13)
                p = 13 + (p - 13) * .5;
            double result = 12.3 * std::exp(.85966 * std::pow(p, 1 / 2.62)) - 30;
            return std::isfinite(result) ? std::max(0., result) : 0.;
        }
        double AccWeight(double a) {
            return a > 0 ? 21.584 * std::pow(a, 1.2163) : 0;
        }
        double TechWeight(double t) {
            if (t <= 0)
                return 0;
            if (t > 12)
                t = 12 + (t - 12) * .33;
            return .1612 * std::pow(t, 1.2477);
        }
        double Power(double s) {
            if (s < 4.803)
                return s <= 2.8818 ? 1.08 : 1.08 - .08 * (s - 2.8818) / (4.803 - 2.8818);
            if (s >= 9.606)
                return .9;
            return s <= 6.7242 ? 1. : 1. - .1 * (s - 6.7242) / (9.606 - 6.7242);
        }
        double Bonus(double s) {
            return s >= 8.6454 ? 1 : 1 + .0063371 * std::pow(8.6454 - std::max(s, 0.), 1.36);
        }
        double RawStars(double p) {
            double b = p / 83.6925;
            return b > 0 ? 1.1653 * std::pow(b, 1.17507) : b;
        }
    }  // namespace

    double Interpolate(Curve const& curve, double x) {
        auto it = std::lower_bound(curve.begin(), curve.end(), x, [](auto const& p, double a) { return p.first > a; });
        auto i = std::clamp<std::ptrdiff_t>(it - curve.begin(), 1, curve.size() - 1);
        auto [x0, y0] = curve[i - 1];
        auto [x1, y1] = curve[i];
        return y0 + (x - x0) / (x1 - x0) * (y1 - y0);
    }
    Curve BuildCurve(double predicted, double sliders, int notes, double span) {
        double noteFactor = notes <= 0 || notes >= 115 ? 1 : std::clamp(1 - .05 * std::log(115. / notes) / std::log(115. / 20), .95, 1.);
        double spanFactor = span <= 0 || span >= 75 ? 1 : std::clamp(1 - .1 * std::log(75 / span) / std::log(75. / 12), .9, 1.);
        double factor = noteFactor * spanFactor;
        bool inRange = predicted > .6 && predicted < .9995;
        Shape shape = inRange ? Parameters(predicted) : Shape{};
        bool modified = inRange && !(shape.a >= .9995 && shape.k == 4.86 && shape.w == .6 && shape.k2 == 2.88 && shape.p2 == 2.46);
        double tilt = inRange ? .2 * sliders : 0;
        if (!modified && tilt == 0) {
            Curve result = base;
            for (auto& p : result)
                p.second *= factor;
            return result;
        }
        std::set<int, std::greater<int>> grid;
        for (auto p : base)
            grid.insert(std::lround(p.first * 1e6));
        for (int i = 0; i <= 70; ++i)
            grid.insert(600000 + 5000 * i);
        for (int i = 0; i <= 100; ++i)
            grid.insert(950000 + 500 * i);
        grid.insert(std::lround((modified ? shape.a : 1) * 1e6));
        static Smooth const smooth;
        Shape const standard;
        Curve result;
        result.reserve(grid.size());
        for (int ix : grid) {
            double x = ix / 1e6, v = Interpolate(base, x);
            if (modified) {
                double b = smooth.Value(x), ref = standard.Value(x);
                v = ref > 0 ? std::min(b, b * shape.Value(x) / ref) : b;
            }
            v *= std::exp(tilt * 100 * std::clamp(x - predicted, -.015, .015));
            result.emplace_back(x, v * factor);
        }
        return result;
    }
    double Stars(Ratings const& r, Curve const& curve) {
        double p = Pass(r.pass) + Interpolate(curve, .96) * AccWeight(r.acc) + std::exp(3.5 * .96) * TechWeight(r.tech);
        double s = RawStars(p);
        if (p <= 0 || s >= 8.6454)
            return s;
        double lo = s, hi = 8.6454;
        while (hi - lo > 1e-6) {
            double mid = (lo + hi) / 2;
            if (mid < RawStars(p * Bonus(mid)))
                lo = mid;
            else
                hi = mid;
        }
        return (lo + hi) / 2;
    }
    std::optional<Context> Prepare(Difficulty const& d, std::vector<std::string> const& mods) {
        if (!d.noteSpan || !std::isfinite(*d.noteSpan) || *d.noteSpan < 0 || d.notes < 0)
            return std::nullopt;
        Ratings ratings = d.base;
        double span = *d.noteSpan, mult = 1;
        std::map<std::string, double> const speeds = {{"ss", .85}, {"fs", 1.2}, {"sf", 1.5}};
        bool selectedSpeed = false;
        for (auto const& mod : mods)
            if (speeds.contains(mod) && d.hasSpeedRatings) {
                auto it = d.speeds.find(mod);
                if (it == d.speeds.end())
                    return std::nullopt;
                if (selectedSpeed)
                    return std::nullopt;
                ratings = it->second;
                span /= speeds.at(mod);
                selectedSpeed = true;
            }
        std::set<std::string> unique(mods.begin(), mods.end());
        for (auto const& mod : unique) {
            if (selectedSpeed && speeds.contains(mod))
                continue;
            auto it = d.modifierValues.find(mod);
            if (it != d.modifierValues.end())
                mult += it->second;
        }
        if (!std::isfinite(ratings.acc) || !std::isfinite(ratings.pass) || !std::isfinite(ratings.tech) || !std::isfinite(ratings.predicted) ||
            !std::isfinite(ratings.sliders) || !std::isfinite(mult) || ratings.acc < 0 || ratings.pass < 0 || ratings.tech < 0)
            return std::nullopt;
        Context c;
        c.curve = BuildCurve(ratings.predicted, ratings.sliders, d.notes, span);
        c.stars = Stars(ratings, c.curve);
        c.power = Power(c.stars);
        c.bonus = 1.023 * Bonus(c.stars);
        c.curve96 = Interpolate(c.curve, .96);
        c.pass = Pass(ratings.pass * mult);
        c.accWeight = AccWeight(ratings.acc * mult);
        c.techWeight = TechWeight(ratings.tech * mult);
        c.noFail = unique.contains("nf");
        c.approximate = !ratings.reportedStars || std::abs(c.stars - *ratings.reportedStars) > .001;
        return c;
    }
    Result Context::Evaluate(double accuracy) const {
        if (noFail || !std::isfinite(accuracy) || accuracy <= 0 || accuracy > 1)
            return {};
        double cv = Interpolate(curve, accuracy);
        if (power != 1 && cv > 0 && curve96 > 0)
            cv = curve96 * std::pow(cv / curve96, power);
        Result r;
        r.pass = pass * bonus;
        r.acc = cv * accWeight * bonus;
        r.tech = std::exp(3.5 * accuracy) * techWeight * bonus;
        r.total = r.pass + r.acc + r.tech;
        return std::isfinite(r.total) ? r : Result{};
    }
    std::string EncodeSegment(std::string const& value) {
        constexpr char hex[] = "0123456789ABCDEF";
        std::string result;
        for (unsigned char c : value) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
                result += c;
            else {
                result += '%';
                result += hex[c >> 4];
                result += hex[c & 15];
            }
        }
        return result;
    }
}  // namespace MetaCore::PPv4
