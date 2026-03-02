#include "gameplay/threats/Owl.h"
#include "core/Constants.h"
#include <SDL2/SDL.h>
#include <cmath>

namespace LightsOut {

Owl::Owl(float worldX, float worldY) : Threat(ThreatType::Owl) {
    position.x = worldX;
    position.y = worldY;
    width  = 12.0f;
    height = 14.0f;
    m_speed = 80.0f;  // fast dive
}

void Owl::playerOnBranch(bool on) {
    m_playerPresent = on;
    if (!on) {
        m_patienceTimer = 0.0f;
        m_attacking     = false;
        m_alerted       = false;
    }
}

void Owl::update(float dt) {
    if (m_frozenTimer > 0.0f) { m_frozenTimer -= dt; return; }

    if (m_playerPresent && !m_attacking) {
        m_patienceTimer += dt;
        if (m_patienceTimer >= OWL_PATIENCE_SECONDS) {
            m_attacking = true;
            m_alerted   = true;
        }
    }

    if (m_attacking) {
        Threat::update(dt);
    }
}

void Owl::render(SDL_Renderer* renderer, float cameraX) {
    float sx = position.x - cameraX;

    // Body
    SDL_SetRenderDrawColor(renderer, 80, 70, 50, 255);
    SDL_FRect body = {sx, position.y + 4.0f, width, height - 4.0f};
    SDL_RenderFillRectF(renderer, &body);

    // Head (round)
    SDL_SetRenderDrawColor(renderer, 100, 90, 60, 255);
    SDL_FRect head = {sx + 2.0f, position.y, 8.0f, 8.0f};
    SDL_RenderFillRectF(renderer, &head);

    // Eyes
    SDL_SetRenderDrawColor(renderer, 240, 200, 50, 255);
    SDL_FRect eye1 = {sx + 3.0f, position.y + 1.5f, 2.5f, 2.5f};
    SDL_FRect eye2 = {sx + 6.0f, position.y + 1.5f, 2.5f, 2.5f};
    SDL_RenderFillRectF(renderer, &eye1);
    SDL_RenderFillRectF(renderer, &eye2);

    // Pupils
    SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
    SDL_RenderDrawPointF(renderer, sx + 4.0f, position.y + 2.5f);
    SDL_RenderDrawPointF(renderer, sx + 7.0f, position.y + 2.5f);

    // Beak
    SDL_SetRenderDrawColor(renderer, 180, 140, 50, 255);
    SDL_RenderDrawLineF(renderer, sx + 5.5f, position.y + 4.0f,
                                   sx + 4.0f, position.y + 6.0f);
    SDL_RenderDrawLineF(renderer, sx + 5.5f, position.y + 4.0f,
                                   sx + 7.0f, position.y + 6.0f);

    // Wings when attacking
    if (m_attacking) {
        SDL_SetRenderDrawColor(renderer, 70, 60, 40, 200);
        SDL_RenderDrawLineF(renderer, sx, position.y + 6.0f, sx - 6.0f, position.y + 2.0f);
        SDL_RenderDrawLineF(renderer, sx + width, position.y + 6.0f,
                                       sx + width + 6.0f, position.y + 2.0f);
    }

    // Patience warning — red eyes when about to attack
    if (m_playerPresent && !m_attacking &&
        m_patienceTimer > OWL_PATIENCE_SECONDS * 0.6f) {
        float t = (m_patienceTimer - OWL_PATIENCE_SECONDS * 0.6f) /
                  (OWL_PATIENCE_SECONDS * 0.4f);
        uint8_t r = static_cast<uint8_t>(240.0f * t);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, r, 30, 30, 150);
        SDL_RenderFillRectF(renderer, &eye1);
        SDL_RenderFillRectF(renderer, &eye2);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

}  // namespace LightsOut
