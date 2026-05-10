#pragma once
#include <GL/gl.h>

enum ShapeType
{
    SHAPE_RECT,
    SHAPE_ELLIPSE
};

class Shape
{
public:
    float x = 0;
    float y = 0;

    float w = 50;
    float h = 50;

    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    float skewX = 0.0f;
    float skewY = 0.0f;

    ShapeType type = SHAPE_RECT;

    void draw();
};
