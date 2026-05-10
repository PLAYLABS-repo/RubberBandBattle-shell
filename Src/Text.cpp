#include "Text.h"
#include "Font.h"
#include <GL/gl.h>

void Text::draw(
    const std::string& text,
    float x,
    float y,
    Font& font,
    TextAlign align,
    float scale
)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font.getTexture());
    glColor4f(1,1,1,1);

    const stbtt_bakedchar* cdata = font.getCharData();

    float startX = x;

    // --- alignment ---
    if (align != LEFT)
    {
        float width = 0;
        for (char c : text)
        {
            if (c >= 32 && c < 128)
                width += cdata[c - 32].xadvance * scale;
        }

        if (align == CENTER) startX -= width / 2;
        if (align == RIGHT)  startX -= width;
    }

    float xpos = startX;
    float ypos = y;

    glBegin(GL_QUADS);

    for (char c : text)
    {
        if (c < 32 || c >= 128) continue;

        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(
            cdata,
            512, 512,
            c - 32,
            &xpos,
            &ypos,
            &q,
            1
        );

        float x0 = q.x0;
        float y0 = q.y0;
        float x1 = q.x1;
        float y1 = q.y1;

        // apply scaling manually
        x1 = x0 + (x1 - x0) * scale;
        y1 = y0 + (y1 - y0) * scale;

        glTexCoord2f(q.s0, q.t0); glVertex2f(x0, y0);
        glTexCoord2f(q.s1, q.t0); glVertex2f(x1, y0);
        glTexCoord2f(q.s1, q.t1); glVertex2f(x1, y1);
        glTexCoord2f(q.s0, q.t1); glVertex2f(x0, y1);
    }

    glEnd();
}
