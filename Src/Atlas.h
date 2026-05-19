#pragma once
#include <map>
#include <string>

struct Frame
{
    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;
};

class Atlas
{
public:
    std::map<std::string, Frame> frames;

    // Load atlas from JSON file.
    // Supports both formats:
    // - Adobe Animate: {"ATLAS": {"SPRITES": [...]}}
    // - TexturePacker: {"frames": {...}}  (Hash or Array format)
    bool load(const char* path);

    // Look up a frame by name.
    // Works with both full paths and bare filenames.
    bool get(const std::string& name, Frame& out) const;
};
