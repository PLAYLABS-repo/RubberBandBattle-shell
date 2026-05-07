#include "Shape.h"
#include <cmath>

void Shape::draw()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(r, g, b, a);

    glPushMatrix();

    // =========================
    // SKEW MATRIX (same style as Sprite)
    // =========================
    float skewMatrix[16] =
    {
        1,        skewY,   0, 0,
        skewX,    1,       0, 0,
        0,        0,       1, 0,
        0,        0,       0, 1
    };

    glMultMatrixf(skewMatrix);

    // =========================
    // RECTANGLE
    // =========================
    if (type == SHAPE_RECT)
    {
        glBegin(GL_QUADS);

        glVertex2f(x,     y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);

        glEnd();
    }

    // =========================
    // ELLIPSE
    // =========================
    else if (type == SHAPE_ELLIPSE)
    {
        const int segments = 32;

        float cx = x + w * 0.5f;
        float cy = y + h * 0.5f;

        float rx = w * 0.5f;
        float ry = h * 0.5f;

        glBegin(GL_TRIANGLE_FAN);

        glVertex2f(cx, cy);

        for (int i = 0; i <= segments; i++)
        {
            float angle = (float)i / segments * 6.2831853f;

            float px = cx + cosf(angle) * rx;
            float py = cy + sinf(angle) * ry;

            glVertex2f(px, py);
        }

        glEnd();
    }

    glPopMatrix();

    glDisable(GL_BLEND);
}
