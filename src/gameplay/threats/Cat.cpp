#include "gameplay/threats/Cat.h"
#include "core/Constants.h"
#include <SDL2/SDL.h>
#include <cmath>

namespace LightsOut {

Cat::Cat(float worldX, float worldY) : Threat(ThreatType::Cat) {
    position.x = worldX;
    position.y = worldY;
    width  = 9.0f;
    height = 7.0f;
    m_speed      = CAT_SPEED;
    m_alertRadius = 30.0f;  // auto-detect player when nearby
}

void Cat::update(float dt) {
    if (m_frozenTimer > 0.0f) { m_frozenTimer -= dt; return; }

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

    // Sleek grey body
    drawSimple(renderer, sx, {100, 100, 110});

    // Head
    SDL_SetRenderDrawColor(renderer, 120, 120, 130, 255);
    SDL_FRect head = {sx + 5.0f, position.y - 3.0f, 6.0f, 6.0f};
    SDL_RenderFillRectF(renderer, &head);

    // Ears (triangular hints)
    SDL_SetRenderDrawColor(renderer, 130, 80, 80, 255);
    SDL_RenderDrawLineF(renderer, sx + 6.0f,  position.y - 3.0f,
                                   sx + 7.0f,  position.y - 6.0f);
    SDL_RenderDrawLineF(renderer, sx + 9.0f,  position.y - 3.0f,
                                   sx + 10.0f, position.y - 6.0f);

    // Glowing eyes
    SDL_SetRenderDrawColor(renderer, 80, 200, 80, 255);
    SDL_RenderDrawPointF(renderer, sx + 7.0f, position.y - 1.0f);
    SDL_RenderDrawPointF(renderer, sx + 9.0f, position.y - 1.0f);

    // Long tail
    SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
    SDL_RenderDrawLineF(renderer, sx, position.y + 4.0f,
                                   sx - 3.0f, position.y - 4.0f);
    SDL_RenderDrawLineF(renderer, sx - 3.0f, position.y - 4.0f,
                                   sx - 1.0f, position.y - 7.0f);
}

}  // namespace LightsOut
