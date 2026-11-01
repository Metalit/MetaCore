#pragma once

#include "pp.hpp"
#include "ppv4.hpp"

namespace MetaCore::PPv4 {
    std::optional<PP::BLSongDiff> ParseMap(std::string const& json);
    std::optional<Context> Prepare(PP::BLSongDiff const& map, std::vector<std::string> const& modifiers);
    void ClearCache();
}
