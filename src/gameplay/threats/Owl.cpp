#include "gameplay/threats/Owl.h"
#include "core/Constants.h"
#include "core/SpriteRegistry.h"
#include <SDL2/SDL.h>
#include <cmath>

namespace LightsOut {

Owl::Owl(float worldX, float worldY) : Threat(ThreatType::Owl) {
    position.x = worldX;
    position.y = worldY;
    width  = 12.0f;
    height = 14.0f;
    m_speed = 80.0f;
    m_chaseDuration = THREAT_CHASE_DURATION - 1.5f;  // owls give up faster
}

void Owl::playerOnFence(bool on) {
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

    const char* sprite;
    if (m_falling) {
        sprite = "owl_fall";
    } else if (m_attacking) {
        sprite = (static_cast<int>(SDL_GetTicks() / 120) % 2 == 0) ? "owl_dive_0" : "owl_dive_1";
    } else if (m_playerPresent) {
        sprite = "owl_watch";
    } else {
        sprite = "owl_perch";
    }

    uint8_t alpha = (m_frozenTimer > 0.0f) ? 140 : 255;
    int sw = 0, sh = 0;
    SpriteRegistry::get(sprite, &sw, &sh);
    if (sh == 0) sh = static_cast<int>(height);
    SpriteRegistry::draw(renderer, sprite,
                         sx - static_cast<float>(sw - (int)width) * 0.5f,
                         position.y + height - static_cast<float>(sh),
                         0.f, 0.f, alpha);

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
