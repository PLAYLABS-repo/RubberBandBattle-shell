#include "FontRenderer.h"
#include <cstdio>

bool FontRenderer::load(const char* path, float pixelHeight)
{
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    unsigned char* ttf = new unsigned char[len];
    fread(ttf, 1, len, f);
    fclose(f);

    const int BW = 512, BH = 512;
    unsigned char* bitmap = new unsigned char[BW * BH];
    stbtt_BakeFontBitmap(ttf, 0, pixelHeight, bitmap, BW, BH, 32, 96, cdata);
    delete[] ttf;

    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, BW, BH, 0,
                 GL_ALPHA, GL_UNSIGNED_BYTE, bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    delete[] bitmap;

    size = pixelHeight;
    return true;
}

void FontRenderer::draw(const char* text, float x, float y,
                        float r, float g, float b, float a)
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, texID);
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    float cx = x, cy = y;
    for (const char* p = text; *p; ++p)
    {
        if (*p < 32 || *p > 127) continue;
        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(cdata, 512, 512, *p - 32, &cx, &cy, &q, 1);
        glTexCoord2f(q.s0, q.t0); glVertex2f(q.x0, q.y0);
        glTexCoord2f(q.s1, q.t0); glVertex2f(q.x1, q.y0);
        glTexCoord2f(q.s1, q.t1); glVertex2f(q.x1, q.y1);
        glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, q.y1);
    }
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glColor4f(1, 1, 1, 1);
}

void FontRenderer::destroy()
{
    if (texID) { glDeleteTextures(1, &texID); texID = 0; }
}
