#include "AABB.h"

// =========================
// Constructors
// =========================
AABB::AABB()
    : x(0), y(0), w(0), h(0)
{
}

AABB::AABB(float x, float y, float w, float h)
    : x(x), y(y), w(w), h(h)
{
}

// =========================
// Intersection (collision)
// =========================
bool AABB::intersects(const AABB& o) const
{
    return !(x + w < o.x ||
             o.x + o.w < x ||
             y + h < o.y ||
             o.y + o.h < y);
}

// =========================
// Point inside box
// =========================
bool AABB::contains(float px, float py) const
{
    return (px >= x &&
            px <= x + w &&
            py >= y &&
            py <= y + h);
}
