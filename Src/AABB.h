#pragma once

class AABB
{
public:
    float x, y;
    float w, h;

    AABB();
    AABB(float x, float y, float w, float h);

    bool intersects(const AABB& other) const;

    bool contains(float px, float py) const;
};
