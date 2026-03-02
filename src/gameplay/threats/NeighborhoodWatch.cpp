#include "gameplay/threats/NeighborhoodWatch.h"
#include "core/Constants.h"
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

    // Car body
    SDL_SetRenderDrawColor(renderer, 30, 30, 80, 255);
    SDL_FRect body = {sx, position.y + 4.0f, width, height - 4.0f};
    SDL_RenderFillRectF(renderer, &body);

    // Roof
    SDL_FRect roof = {sx + 4.0f, position.y, width - 8.0f, 6.0f};
    SDL_RenderFillRectF(renderer, &roof);

    // Windows
    SDL_SetRenderDrawColor(renderer, 100, 140, 180, 200);
    SDL_FRect win = {sx + 5.0f, position.y + 1.0f, width - 10.0f, 4.0f};
    SDL_RenderFillRectF(renderer, &win);

    // Wheels
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_FRect wheel1 = {sx + 2.0f, position.y + height - 4.0f, 5.0f, 4.0f};
    SDL_FRect wheel2 = {sx + width - 7.0f, position.y + height - 4.0f, 5.0f, 4.0f};
    SDL_RenderFillRectF(renderer, &wheel1);
    SDL_RenderFillRectF(renderer, &wheel2);

    // Roof lights (flashing red/blue)
    Uint32 ticks = SDL_GetTicks();
    bool  redPhase = (ticks / 300) % 2 == 0;
    SDL_SetRenderDrawColor(renderer,
        redPhase ? 255 : 50,
        30,
        redPhase ? 50 : 255, 220);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FRect light = {sx + 9.0f, position.y - 2.0f, 6.0f, 3.0f};
    SDL_RenderFillRectF(renderer, &light);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Spotlight cone ahead of car
    SDL_SetRenderDrawColor(renderer, 255, 255, 180, 40);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < static_cast<int>(SPOTLIGHT_WIDTH); i += 3) {
        float alpha = 40.0f * (1.0f - static_cast<float>(i) / SPOTLIGHT_WIDTH);
        SDL_SetRenderDrawColor(renderer, 255, 255, 180, static_cast<uint8_t>(alpha));
        SDL_RenderDrawLineF(renderer,
            sx + width, position.y + height - 6.0f,
            sx + width + i, LANE_GROUND_Y);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

}  // namespace LightsOut
