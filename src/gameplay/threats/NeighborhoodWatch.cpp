#include "gameplay/threats/NeighborhoodWatch.h"
#include "core/Constants.h"
#include "core/SpriteRegistry.h"
#include "gameplay/Collision.h"
#include <SDL2/SDL.h>

namespace LightsOut {

NeighborhoodWatch::NeighborhoodWatch(float worldX)
    : Threat(ThreatType::NeighborhoodWatch)
    , m_scrollX(worldX)
{
    position.x = worldX;
    position.y = LANE_GROUND_Y - 14.0f;
    width  = 24.0f;
    height = 14.0f;
    m_speed = WATCH_CAR_SPEED;
    // The car drifts slightly slower than world scroll — appears to move toward player
}

void NeighborhoodWatch::update(float dt) {
    if (m_frozenTimer > 0.0f) { m_frozenTimer -= dt; return; }
    // Car scrolls naturally with the world (no self-movement needed;
    // GameWorld removes it when off-screen)
    position.y = LANE_GROUND_Y - height;
}

bool NeighborhoodWatch::detectsGroundPlayer(const Rect& playerBounds) const {
    // Spotlight extends ahead of the car
    Rect spotlight = {
        position.x + width,
        LANE_GROUND_Y - SPOTLIGHT_HEIGHT,
        SPOTLIGHT_WIDTH,
        SPOTLIGHT_HEIGHT
    };
    return Collision::overlaps(spotlight, playerBounds);
}

void NeighborhoodWatch::render(SDL_Renderer* renderer, float cameraX) {
    float sx = position.x - cameraX;

    // Car body sprite (bottom-aligned)
    uint8_t alpha = (m_frozenTimer > 0.0f) ? 140 : 255;
    int sw = 0, sh = 0;
    SpriteRegistry::get("watchcar_body", &sw, &sh);
    if (sh == 0) sh = static_cast<int>(height);
    float drawX = sx - static_cast<float>(sw - (int)width) * 0.5f;
    float drawY = position.y + height - static_cast<float>(sh);
    SpriteRegistry::draw(renderer, "watchcar_body", drawX, drawY, 0.f, 0.f, alpha);

    // Flashing roof lights (red/blue alternating)
    bool redPhase = (SDL_GetTicks() / 300) % 2 == 0;
    const char* lightSprite = redPhase ? "watchcar_lights_red" : "watchcar_lights_blue";
    SpriteRegistry::draw(renderer, lightSprite, drawX, drawY, 0.f, 0.f, alpha);

    // Spotlight ahead of car
    SpriteRegistry::draw(renderer, "watchcar_spotlight",
                         sx + width, position.y + height - 8.0f,
                         0.f, 0.f, 100);

    // Frozen overlay
    if (m_frozenTimer > 0.0f) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 80, 160, 255, 80);
        SDL_FRect tint = {sx, position.y, width, height};
        SDL_RenderFillRectF(renderer, &tint);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

}  // namespace LightsOut
