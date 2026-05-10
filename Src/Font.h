#pragma once

#include <string>
#include <GL/gl.h>
#include "stb_truetype.h"

class Font
{
public:
    bool load(const std::string& path, int size);

    const stbtt_bakedchar* getCharData() const;
    GLuint getTexture() const;

private:
    stbtt_bakedchar cdata[96]; // ASCII 32–127
    GLuint texture;
};
