#include "gameplay/threats/Threat.h"
#include "core/Constants.h"
#include <SDL2/SDL.h>
#include <cmath>
#include <algorithm>

namespace LightsOut {

Threat::Threat(ThreatType type) : Entity(TAG_THREAT), m_type(type) {}

void Threat::update(float dt) {
    if (m_frozenTimer > 0.0f) {
        m_frozenTimer = std::max(0.0f, m_frozenTimer - dt);
        return;
    }
    if (m_distracted) return;

    if (m_alerted && m_speed > 0.0f) {
        // Move toward target
        float dx = m_targetPos.x - position.x;
        float dy = m_targetPos.y - position.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > 2.0f) {
            velocity.x = (dx / dist) * m_speed;
            velocity.y = (dy / dist) * m_speed;
            position.x += velocity.x * dt;
            position.y += velocity.y * dt;
        }
    }
}

void Threat::render(SDL_Renderer* renderer, float cameraX) {
    float sx = position.x - cameraX;
    drawSimple(renderer, sx, {200, 80, 80});
}

void Threat::alert(const Vec2& playerPos) {
    m_alerted   = true;
    m_targetPos = playerPos;
}

void Threat::freeze(float seconds) {
    m_frozenTimer = seconds;
}

void Threat::distract(const Vec2& decoyPos) {
    m_distracted = true;
    m_targetPos  = decoyPos;
}

bool Threat::caughtPlayer(const Rect& playerBounds) const {
    return Rect{position.x, position.y, width, height}.intersects(playerBounds);
}

void Threat::drawSimple(SDL_Renderer* renderer, float screenX, Color c) const {
    if (m_frozenTimer > 0.0f) {
        // Frozen: draw blue tint
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 100, 180, 255, 200);
    } else {
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
    }
    SDL_FRect rect = {screenX, position.y, width, height};
    SDL_RenderFillRectF(renderer, &rect);

    // Alert indicator
    if (m_alerted) {
        SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
        SDL_RenderDrawPointF(renderer, screenX + width * 0.5f, position.y - 3.0f);
        SDL_RenderDrawPointF(renderer, screenX + width * 0.5f - 1.0f, position.y - 5.0f);
        SDL_RenderDrawPointF(renderer, screenX + width * 0.5f + 1.0f, position.y - 5.0f);
    }
    if (m_frozenTimer > 0.0f) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

}  // namespace LightsOut
