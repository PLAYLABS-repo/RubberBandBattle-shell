
#pragma once
// =============================================================
// Light.h — directional and point light descriptors
// Consumed by Playlabs_DrawModel() → fed into GL_LIGHT0..7
// =============================================================

struct Light
{
    enum Type { DIRECTIONAL, POINT };
    Type type = DIRECTIONAL;

    // Directional light — world-space direction (need not be normalised)
    float dirX = 0, dirY = -1, dirZ = 0;

    // Point light — world-space position
    float posX = 0, posY = 10, posZ = 0;

    // Colour (0-1 per channel)
    float r = 1, g = 1, b = 1;

    // Brightness multiplier
    float intensity = 1.0f;

    // Point-light attenuation:  1 / (1 + linear*d + quadratic*d²)
    float linear    = 0.09f;
    float quadratic = 0.032f;
};
