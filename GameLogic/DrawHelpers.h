
#pragma once
#include "AbsolutEngine.h"
#include <GL/gl.h>
#include <cmath>
#include "Constants.h"

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


