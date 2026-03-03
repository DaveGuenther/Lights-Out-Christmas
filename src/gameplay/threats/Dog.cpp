#include "gameplay/threats/Dog.h"
#include "core/Constants.h"
#include <SDL2/SDL.h>
#include <cmath>

namespace LightsOut {

Dog::Dog(float worldX, float worldY, float chainRadius)
    : Threat(ThreatType::Dog)
    , m_anchorPos(worldX, worldY)
    , m_chainRadius(chainRadius)
{
    position.x = worldX;
    position.y = worldY;
    width  = 10.0f;
    height = 8.0f;
    m_speed = 45.0f;
    m_chaseDuration = THREAT_CHASE_DURATION;
}

void Dog::alert(const Vec2& playerPos) {
    Threat::alert(playerPos);
    m_barking = true;
}

void Dog::distract(const Vec2& decoyPos) {
    // Dog chases the decoy nut
    m_distracted = true;
    m_targetPos  = decoyPos;
    m_barking    = false;
}

void Dog::update(float dt) {
    if (m_frozenTimer > 0.0f) {
        m_frozenTimer -= dt;
        return;
    }

    // Bark timer
    if (m_barking) {
        m_barkTimer += dt;
    }

    if (!m_alerted && !m_distracted) return;

    // Dog is chain-constrained — can only reach within m_chainRadius
    Vec2  target = m_targetPos;
    float dx = target.x - m_anchorPos.x;
    float dy = target.y - m_anchorPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist > m_chainRadius) {
        // Clamp target to chain boundary
        target.x = m_anchorPos.x + (dx / dist) * m_chainRadius;
        target.y = m_anchorPos.y + (dy / dist) * m_chainRadius;
    }

    m_targetPos = target;
    Threat::update(dt);

    // Stay on ground
    position.y = LANE_GROUND_Y - height;
}

void Dog::render(SDL_Renderer* renderer, float cameraX) {
    float sx = position.x - cameraX;

    // Chain
    float asx = m_anchorPos.x - cameraX;
    SDL_SetRenderDrawColor(renderer, 120, 100, 60, 180);
    SDL_RenderDrawLineF(renderer, asx, LANE_GROUND_Y - 2.0f, sx + 5.0f, position.y + 4.0f);

    // Body
    drawSimple(renderer, sx, {140, 90, 40});

    // Head (slightly in front)
    SDL_SetRenderDrawColor(renderer, 160, 100, 50, 255);
    SDL_FRect head = {sx + 7.0f, position.y - 2.0f, 6.0f, 6.0f};
    SDL_RenderFillRectF(renderer, &head);

    // Nose
    SDL_SetRenderDrawColor(renderer, 40, 25, 20, 255);
    SDL_FRect nose = {sx + 12.0f, position.y + 1.0f, 2.0f, 2.0f};
    SDL_RenderFillRectF(renderer, &nose);

    // Bark indicator
    if (m_barking && static_cast<int>(m_barkTimer * 4.0f) % 2 == 0) {
        SDL_SetRenderDrawColor(renderer, 255, 220, 100, 200);
        SDL_RenderDrawLineF(renderer, sx + 13.0f, position.y - 1.0f,
                                       sx + 16.0f, position.y - 4.0f);
        SDL_RenderDrawLineF(renderer, sx + 14.0f, position.y + 1.0f,
                                       sx + 17.0f, position.y - 2.0f);
    }

    // Tail
    SDL_SetRenderDrawColor(renderer, 140, 90, 40, 255);
    SDL_RenderDrawLineF(renderer, sx, position.y + 2.0f, sx - 4.0f, position.y - 2.0f);
}

}  // namespace LightsOut
