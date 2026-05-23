#pragma once

enum class CollisionSide { None, Top, Bottom, Left, Right };

class AABB
{
public:
    float x, y;
    float w, h;

    AABB();
    AABB(float x, float y, float w, float h);

    bool intersects(const AABB& other) const;
    bool contains(float px, float py) const;
    CollisionSide getCollisionSide(const AABB& other) const;

    float left()   const { return x; }
    float right()  const { return x + w; }
    float top()    const { return y; }
    float bottom() const { return y + h; }
};
