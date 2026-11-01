#pragma once
#include <fstream>
#include <string>
inline bool fileexists(std::string_view path) {
    return std::ifstream(std::string(path)).good();
}
inline std::string readfile(std::string_view path) {
    std::ifstream stream{std::string(path)};
    return {std::istreambuf_iterator<char>(stream), {}};
}
inline void writefile(std::string_view path, std::string_view data) {
    std::ofstream{std::string(path)} << data;
}
