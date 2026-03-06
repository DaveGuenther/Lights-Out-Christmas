#include "gameplay/threats/Homeowner.h"
#include "core/Constants.h"
#include "core/SpriteRegistry.h"
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

    // Pick sprite based on personality and alert state
    const char* sprite;
    if (m_personality == Personality::GrumpyOldMan) {
        if (m_alerted) {
            Uint32 frame = (SDL_GetTicks() / 220) % 2;
            sprite = frame ? "homeowner_grumpy_run_1" : "homeowner_grumpy_run_0";
        } else {
            sprite = "homeowner_grumpy_idle";
        }
    } else {
        // DadInPajamas
        if (m_givenUp) {
            sprite = "homeowner_dad_givingup";
        } else if (m_alerted) {
            Uint32 frame = (SDL_GetTicks() / 200) % 2;
            sprite = frame ? "homeowner_dad_run_1" : "homeowner_dad_run_0";
        } else {
            sprite = "homeowner_dad_idle";
        }
    }

    // Homeowner sprites are 10x18 — draw bottom-aligned to entity bottom
    uint8_t alpha = (m_frozenTimer > 0.0f) ? 140 : 255;
    SpriteRegistry::draw(renderer, sprite,
                         sx, position.y + height - 18.0f,
                         0.f, 0.f, alpha);

    // Frozen blue tint overlay
    if (m_frozenTimer > 0.0f) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 80, 160, 255, 80);
        SDL_FRect tint = {sx, position.y, width, height};
        SDL_RenderFillRectF(renderer, &tint);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    // Alert exclamation indicator
    if (m_alerted && !m_givenUp) {
        SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
        SDL_RenderDrawPointF(renderer, sx + width * 0.5f, position.y - 3.0f);
        SDL_RenderDrawPointF(renderer, sx + width * 0.5f, position.y - 5.0f);
    }
}

}  // namespace LightsOut
