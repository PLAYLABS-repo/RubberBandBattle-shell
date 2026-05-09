#pragma once
#include <GL/gl.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct FontRenderer
{
    stbtt_bakedchar cdata[96];
    GLuint          texID = 0;
    float           size  = 0.0f;

    bool load(const char* path, float pixelHeight);
    void draw(const char* text, float x, float y,
              float r = 1.f, float g = 1.f, float b = 1.f, float a = 1.f);
    void destroy();
};
