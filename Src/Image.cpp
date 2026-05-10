#include "Image.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool Image::load(const char* path) {
    int n;
    // Use the class members 'width' and 'height' instead of 'w' and 'h'
    unsigned char* data = stbi_load(path, &width, &height, &n, 4);

    if (!data) return false;

    // Use 'textureID' instead of 'tex'
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Setup scaling filters (Required for the texture to render)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return true;
}
