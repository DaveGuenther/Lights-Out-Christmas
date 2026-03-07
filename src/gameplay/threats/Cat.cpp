#include "gameplay/threats/Cat.h"
#include "core/Constants.h"
#include "core/SpriteRegistry.h"
#include <SDL2/SDL.h>
#include <cmath>

namespace LightsOut {

Cat::Cat(float worldX, float worldY) : Threat(ThreatType::Cat) {
    position.x = worldX;
    position.y = worldY;
    width  = 9.0f;
    height = 7.0f;
    m_speed       = CAT_SPEED;
    m_chaseDuration = THREAT_CHASE_DURATION + 1.0f;
    m_alertRadius = 30.0f;  // auto-detect player when nearby
}

void Cat::update(float dt) {
    if (m_frozenTimer > 0.0f) { m_frozenTimer -= dt; return; }

    // Fall off screen when chase duration expires
    if (m_falling) { Threat::update(dt); return; }

    // Stalking phase before engaging
    if (!m_alerted) {
        m_stalkerTimer += dt;
        return;
    }

    // After stalk delay, chase relentlessly
    Threat::update(dt);
}

void Cat::render(SDL_Renderer* renderer, float cameraX) {
    float sx = position.x - cameraX;

    const char* sprite;
    if (m_falling) {
        sprite = "cat_fall";
    } else if (!m_alerted) {
        sprite = "cat_stalk";
    } else {
        sprite = (static_cast<int>(SDL_GetTicks() / 150) % 2 == 0) ? "cat_run_0" : "cat_run_1";
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
