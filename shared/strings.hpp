#pragma once

#include <string>
#include <fmt/format.h>

namespace MetaCore::Strings {
    /// @brief Santizes a path to remove all invalid characters with an aggressive whitelist
    /// @param path The unsanitized path
    /// @return The sanitized path
    std::string SanitizedPath(std::string const& path);

    /// @brief Converts a number of seconds to a string in the form minutes:seconds
    /// @param seconds The total number of seconds
    /// @param hours If hours should be included in the string as well
    /// @return The formatted string
    std::string SecondsToString(int seconds, bool hours = false);

    /// @brief Converts a time in the past (such as a file modified time) to a string of the largest time unit
    /// @param pastTime The time in the past, in seconds
    /// @return The formatted string, for example "1 day ago"
    std::string TimeAgoString(long pastTime);

    /// @brief Compares two strings for equality, ignoring case
    /// @param a The first string to compare
    /// @param b The second string to compare
    /// @return If the strings are equal, ignoring case
    bool IEquals(std::string const& a, std::string const& b);

    /// @brief Formats a number with a specific number of decimal places
    /// @param value The number to format
    /// @param decimals The fixed number of decimals to write
    /// @return The formatted string
    inline std::string FormatDecimals(double value, int decimals) {
        return fmt::format("{:.{}f}", value, decimals);
    }
}
