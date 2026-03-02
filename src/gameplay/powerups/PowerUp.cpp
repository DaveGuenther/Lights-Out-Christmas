#include "gameplay/powerups/PowerUp.h"
#include <SDL2/SDL.h>
#include <cmath>

namespace LightsOut {

PowerUp::PowerUp(PowerUpType type, float worldX, float worldY)
    : Entity(TAG_POWERUP), m_type(type)
{
    position.x = worldX;
    position.y = worldY;
    width  = 10.0f;
    height = 10.0f;
}

void PowerUp::update(float dt) {
    if (m_collected) { alive = false; return; }
    m_bobTimer += dt;
    // Bob up/down
    position.y += std::sin(m_bobTimer * 4.0f) * 0.3f;
}

void PowerUp::render(SDL_Renderer* renderer, float cameraX) {
    if (m_collected) return;
    float sx = position.x - cameraX;

    // Draw a small pulsing gem
    float pulse = 0.8f + 0.2f * std::sin(m_bobTimer * 6.0f);
    uint8_t alpha = static_cast<uint8_t>(200 * pulse);

    // Glow
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, m_color.r, m_color.g, m_color.b, alpha / 3);
    SDL_FRect glow = {sx - 3.0f, position.y - 3.0f, width + 6.0f, height + 6.0f};
    SDL_RenderFillRectF(renderer, &glow);

    // Core
    SDL_SetRenderDrawColor(renderer, m_color.r, m_color.g, m_color.b, alpha);
    SDL_FRect core = {sx, position.y, width, height};
    SDL_RenderFillRectF(renderer, &core);

    // Sparkle
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
    SDL_RenderDrawPointF(renderer, sx + 2.0f, position.y + 2.0f);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

}  // namespace LightsOut
