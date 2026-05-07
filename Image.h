#pragma once
#include <GL/gl.h>

class Image {
public:
    unsigned int textureID = 0;
    int width = 0;
    int height = 0;

    bool load(const char* path);

    // Helper methods for the Sprite class
    void bind() { glBindTexture(GL_TEXTURE_2D, textureID); }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};
