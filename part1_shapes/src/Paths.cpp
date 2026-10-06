// =============================================================================
// Paths.cpp
// =============================================================================
#include "Paths.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace fs = std::filesystem;

namespace Paths {

std::string resolve(const std::string& relative) {
    for (const char* prefix : {"", "../"}) {
        fs::path p = fs::path(prefix) / relative;
        if (fs::exists(p)) return p.string();
    }
    return relative;   // let the caller report "cannot open"
}

std::vector<std::string> listFiles(const std::string& relativeDir, const std::vector<std::string>& extensions) {
    std::vector<std::string> out;
    std::error_code ec;
    fs::path dir = resolve(relativeDir);
    if (!fs::is_directory(dir, ec)) return out;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
        if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end())
            out.push_back(entry.path().string());
    }
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace Paths
