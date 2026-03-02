#pragma once
#include "core/Types.h"
#include <SDL2/SDL.h>
#include <cstdint>

namespace LightsOut {

class Entity {
public:
    explicit Entity(uint32_t tags = TAG_NONE);
    virtual ~Entity() = default;

    virtual void update(float dt) = 0;
    virtual void render(SDL_Renderer* renderer, float cameraX) = 0;

    // World-space position and bounds
    Vec2 position;
    Vec2 velocity;
    float width  = 0.0f;
    float height = 0.0f;

    uint32_t tags = TAG_NONE;
    bool     alive = true;      // false → remove from world

    Rect bounds() const { return {position.x, position.y, width, height}; }
    Vec2 center() const { return {position.x + width * 0.5f, position.y + height * 0.5f}; }

    // World-space X that has scrolled off the left edge
    bool isOffscreen(float cameraX, float renderWidth) const;
};

}  // namespace LightsOut
