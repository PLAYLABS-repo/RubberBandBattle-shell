#pragma once
#include "PlaylabsGL.h"
#include <GL/gl.h>
#include <cmath>
#include "Constants.h"

// =============================================================
// DRAW HELPERS
// =============================================================

static void drawRect(float x, float y, float w, float h,
                     float r, float g, float b, float a = 1.0f)
{
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x,     y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x,     y + h);
    glEnd();
    glColor4f(1, 1, 1, 1);
}

static void drawLine(float x0, float y0, float x1, float y1,
                     float r, float g, float b, float a = 1.0f, float width = 2.0f)
{
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(width);
    glColor4f(r, g, b, a);
    glBegin(GL_LINES);
    glVertex2f(x0, y0);
    glVertex2f(x1, y1);
    glEnd();
    glLineWidth(1.0f);
    glColor4f(1, 1, 1, 1);
}

static void drawTriangle(float ax, float ay, float bx, float by,
                          float cx, float cy,
                          float r, float g, float b, float a = 1.0f)
{
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLES);
    glVertex2f(ax, ay);
    glVertex2f(bx, by);
    glVertex2f(cx, cy);
    glEnd();
    glColor4f(1, 1, 1, 1);
}

static void drawImageStretched(Image* img, float x, float y,
                                float w, float h, float alpha = 1.0f)
{
    if (!img) return;
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, img->textureID);
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(x,     y);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(x + w, y);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(x + w, y + h);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(x,     y + h);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glColor4f(1, 1, 1, 1);
}

static void drawProgressBar(Image* fillImg, Image* borderImg,
                             float x, float y, float w, float h,
                             float fill, float alpha = 1.0f)
{
    if (fillImg && fill > 0.0f)
    {
        float fw = w * fill;
        float u1 = fill;

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, fillImg->textureID);
        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(x,      y);
        glTexCoord2f(u1,   0.0f); glVertex2f(x + fw, y);
        glTexCoord2f(u1,   1.0f); glVertex2f(x + fw, y + h);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(x,      y + h);
        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        glColor4f(1, 1, 1, 1);
    }

    drawImageStretched(borderImg, x, y, w, h, alpha);
}

// =============================================================
// CAMERA HELPERS
// =============================================================

static void applyCamera2D(Camera& cam, int sw, int sh)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float visW = sw / cam.zoom;
    float visH = sh / cam.zoom;
    glOrtho(cam.position.x, cam.position.x + visW,
            cam.position.y + visH, cam.position.y,
            -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void applyScreenSpace(int sw, int sh)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, sw, sh, 0, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void screenToWorld(float sx, float sy,
                           const Camera& cam, int sw, int sh,
                           float& wx, float& wy)
{
    float visW = sw / cam.zoom;
    float visH = sh / cam.zoom;
    wx = cam.position.x + (sx / sw) * visW;
    wy = cam.position.y + (sy / sh) * visH;
}

// =============================================================
// ARC PREVIEW
// =============================================================

static void drawArcPreview(float ox, float oy,
                            float vx, float vy,
                            float r, float g, float b)
{
    float px = ox, py = oy;
    float pvx = vx, pvy = vy;
    float endX = ox, endY = oy;
    float prevX = ox, prevY = oy;

    for (int i = 0; i < ARC_SEGMENTS; ++i)
    {
        float nx  = px + pvx * ARC_STEP_T;
        float ny  = py + pvy * ARC_STEP_T + 0.5f * BULLET_GRAVITY * ARC_STEP_T * ARC_STEP_T;
        float nvy = pvy + BULLET_GRAVITY * ARC_STEP_T;

        float alpha = 1.0f - (float)i / ARC_SEGMENTS;
        drawLine(px, py, nx, ny, r, g, b, alpha, 2.5f);

        prevX = px; prevY = py;
        endX  = nx; endY  = ny;
        px  = nx; py  = ny;
        pvy = nvy;
    }

    float dx = endX - prevX;
    float dy = endY - prevY;
    float len = sqrtf(dx * dx + dy * dy);

    if (len > 0.0001f)
    {
        dx /= len;
        dy /= len;

        float tx  = endX + dx * ARROW_SIZE;
        float ty  = endY + dy * ARROW_SIZE;
        float px1 = endX - dy * ARROW_SIZE * 0.5f;
        float py1 = endY + dx * ARROW_SIZE * 0.5f;
        float px2 = endX + dy * ARROW_SIZE * 0.5f;
        float py2 = endY - dx * ARROW_SIZE * 0.5f;

        drawTriangle(tx, ty, px1, py1, px2, py2, r, g, b, 1.0f);
    }
}
