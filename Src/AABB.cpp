#include "AABB.h"
#include <algorithm>

AABB::AABB()
    : x(0), y(0), w(0), h(0) {}

AABB::AABB(float x, float y, float w, float h)
    : x(x), y(y), w(w), h(h) {}

bool AABB::intersects(const AABB& other) const
{
    if (right()  <= other.left())   return false;
    if (left()   >= other.right())  return false;
    if (bottom() <= other.top())    return false;
    if (top()    >= other.bottom()) return false;
    return true;
}

bool AABB::contains(float px, float py) const
{
    return px >= left()  && px <= right()
        && py >= top()   && py <= bottom();
}

CollisionSide AABB::getCollisionSide(const AABB& other) const
{
    if (!intersects(other)) return CollisionSide::None;

    // Overlap depth on each axis
    float overlapLeft   = right()  - other.left();
    float overlapRight  = other.right()  - left();
    float overlapTop    = bottom() - other.top();
    float overlapBottom = other.bottom() - top();

    // Smallest penetration wins
    float minOverlap = std::min({ overlapLeft, overlapRight, overlapTop, overlapBottom });

    if (minOverlap == overlapTop)    return CollisionSide::Top;
    if (minOverlap == overlapBottom) return CollisionSide::Bottom;
    if (minOverlap == overlapLeft)   return CollisionSide::Left;
                                     return CollisionSide::Right;
}
