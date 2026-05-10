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

    bool load(const char* path);

    bool get(const std::string& name, Frame& out) const;
};
