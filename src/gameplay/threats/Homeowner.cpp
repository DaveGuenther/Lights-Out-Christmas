#include "gameplay/threats/Homeowner.h"
#include "core/Constants.h"
#include <SDL2/SDL.h>
#include <algorithm>

namespace LightsOut {

Homeowner::Homeowner(float worldX, float worldY, Personality p)
    : Threat(p == Personality::GrumpyOldMan
             ? ThreatType::Homeowner_Grumpy
             : ThreatType::Homeowner_Dad)
    , m_personality(p)
{
    position.x = worldX;
    position.y = worldY;
    width  = 10.0f;
    height = 16.0f;

    m_chaseDuration = THREAT_CHASE_DURATION + 2.0f;
    m_speed = (p == Personality::GrumpyOldMan)
              ? HOMEOWNER_GRUMPY_SPEED
              : HOMEOWNER_DAD_SPEED;
}

void Homeowner::alert(const Vec2& playerPos) {
    Threat::alert(playerPos);
    m_givenUp      = false;
    m_giveUpTimer  = DAD_GIVE_UP_TIME;
}

void Homeowner::update(float dt) {
    if (m_givenUp) return;

    // Dad gives up after a while
    if (m_personality == Personality::DadInPajamas && m_alerted) {
        m_giveUpTimer -= dt;
        if (m_giveUpTimer <= 0.0f) {
            m_givenUp = true;
            m_alerted = false;
            alive     = false;  // remove from world
            return;
        }
    }

    // Update target to follow player X (homeowners stay on ground)
    m_targetPos.y = LANE_GROUND_Y - height * 0.5f;

    Threat::update(dt);
}

void Homeowner::render(SDL_Renderer* renderer, float cameraX) {
    float sx = position.x - cameraX;

    // Body color: grumpy = dark red/brown, dad = blue pajamas
    Color bodyColor = (m_personality == Personality::GrumpyOldMan)
                      ? Color{80, 40, 30}
                      : Color{50, 70, 140};

    drawSimple(renderer, sx, bodyColor);

    // Head
    SDL_SetRenderDrawColor(renderer, 200, 160, 120, 255);
    SDL_FRect head = {sx + 2.0f, position.y, 6.0f, 6.0f};
    SDL_RenderFillRectF(renderer, &head);

    // Angry eyebrow
    if (m_alerted) {
        SDL_SetRenderDrawColor(renderer, 60, 20, 10, 255);
        SDL_RenderDrawLineF(renderer, sx + 3.0f, position.y + 1.5f,
                                       sx + 5.0f, position.y + 2.5f);
        SDL_RenderDrawLineF(renderer, sx + 5.0f, position.y + 1.5f,
                                       sx + 7.0f, position.y + 2.5f);
    }

    // Weapon (flashlight or newspaper)
    SDL_SetRenderDrawColor(renderer, 200, 200, 50, 255);
    SDL_RenderDrawLineF(renderer, sx + 9.0f, position.y + 6.0f,
                                   sx + 13.0f, position.y + 4.0f);
}

}  // namespace LightsOut
