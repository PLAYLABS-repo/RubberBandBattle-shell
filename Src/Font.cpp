// Font.cpp
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "Font.h"

#include <fstream>
#include <vector>
#include <GL/gl.h>

static unsigned char fontBitmap[512 * 512]; // atlas

const stbtt_bakedchar* Font::getCharData() const { return cdata; }
GLuint Font::getTexture() const { return texture; }

bool Font::load(const std::string& path, int size)
{
    // Load font file into memory
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)),
                                       std::istreambuf_iterator<char>());

    // Bake font into bitmap
    int result = stbtt_BakeFontBitmap(
        buffer.data(),
        0,
        (float)size,
        fontBitmap,
        512, 512,
        32, 96, // ASCII range
        cdata
    );

    if (result <= 0)
        return false;

    // Create OpenGL texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_ALPHA,
        512, 512,
        0,
        GL_ALPHA,
        GL_UNSIGNED_BYTE,
        fontBitmap
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return true;
}
