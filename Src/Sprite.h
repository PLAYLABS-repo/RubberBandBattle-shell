#pragma once
#include <string>
#include "Vec2.h"
#include "AABB.h"

class Image;
class Atlas;
class Camera;

class Sprite
{
public:
    Image* image = nullptr;
    Atlas* atlas = nullptr;

    std::string frameName;

    // =========================
    // TRANSFORM
    // =========================
    Vec2 position = {0, 0};
    Vec2 scale = {1, 1};
    float rotation = 0.0f;

    float skewX = 0.0f;
    float skewY = 0.0f;

    float alpha = 1.0f;

    // 🔥 REAL PIVOT (FROM TIMELINE)
    Vec2 pivot = {0, 0};

    // =========================
    // TARGETS (LINEAR)
    // =========================
    Vec2 targetPosition = {0, 0};
    Vec2 targetScale = {1, 1};
    float targetRotation = 0.0f;

    float moveSpeed = 400.0f;
    float scaleSpeed = 6.0f;
    float rotateSpeed = 6.0f;

    // =========================
    void moveTo(float x, float y);
    void scaleTo(float w, float h);
    void rotateTo(float degrees);

    void update(float dt);

    AABB getAABB() const;
    void draw(Camera& cam);
};
