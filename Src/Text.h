#pragma once
#include <string>

class Font;

enum TextAlign
{
    LEFT,
    CENTER,
    RIGHT
};

class Text
{
public:
    static void draw(
        const std::string& text,
        float x,
        float y,
        Font& font,
        TextAlign align,
        float scale = 1.0f
    );
};
