#include "Atlas.h"
#include "json.hpp"
#include <fstream>

using json = nlohmann::json;

// Strip "folder/subfolder/" prefix — Animate exports full paths but
// TimelineAnimator (and most lookups) only use the bare name.
static std::string stripPath(const std::string& name)
{
    size_t slash = name.find_last_of("/\\");
    if (slash != std::string::npos)
        return name.substr(slash + 1);
    return name;
}

bool Atlas::load(const char* path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    json j;
    try { file >> j; }
    catch (...) { return false; }

    if (!j.contains("ATLAS") || !j["ATLAS"].contains("SPRITES"))
        return false;

    for (auto& s : j["ATLAS"]["SPRITES"])
    {
        if (!s.contains("SPRITE")) continue;
        auto& sp = s["SPRITE"];

        Frame f;
        f.x = sp.value("x", 0.0f);
        f.y = sp.value("y", 0.0f);
        f.w = sp.value("w", 0.0f);
        f.h = sp.value("h", 0.0f);

        std::string fullName = sp.value("name", std::string(""));
        if (fullName.empty()) continue;

        // Store under the bare name (primary lookup)
        frames[stripPath(fullName)] = f;

        // Also store under the full path so nothing breaks if
        // a caller passes the original unstripped name
        if (fullName != stripPath(fullName))
            frames[fullName] = f;
    }

    return true;
}

bool Atlas::get(const std::string& name, Frame& out) const
{
    auto it = frames.find(name);
    if (it != frames.end())
    {
        out = it->second;
        return true;
    }

    // Fallback: try stripping the path from the requested name too
    std::string bare = name;
    size_t slash = bare.find_last_of("/\\");
    if (slash != std::string::npos)
        bare = bare.substr(slash + 1);

    it = frames.find(bare);
    if (it != frames.end())
    {
        out = it->second;
        return true;
    }

    return false;
}
