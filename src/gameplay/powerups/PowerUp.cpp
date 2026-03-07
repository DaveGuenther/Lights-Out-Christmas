#include "gameplay/powerups/PowerUp.h"
#include "core/SpriteRegistry.h"
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

static const char* powerUpSpriteName(PowerUpType type) {
    switch (type) {
        case PowerUpType::AcornStash:  return "powerup_acorn";
        case PowerUpType::WinterCoat:  return "powerup_coat";
        case PowerUpType::ShadowMode:  return "powerup_shadow";
        case PowerUpType::SuperChomp:  return "powerup_chomp";
        case PowerUpType::DecoyNut:    return "powerup_decoy";
        case PowerUpType::IcePatch:    return "powerup_ice";
        case PowerUpType::DoubleTail:  return "powerup_doubletail";
        case PowerUpType::FrenzyMode:  return "powerup_frenzy";
        default:                       return "powerup_acorn";
    }
}

void PowerUp::render(SDL_Renderer* renderer, float cameraX) {
    if (m_collected) return;
    float sx = position.x - cameraX;

    float pulse = 0.8f + 0.2f * std::sin(m_bobTimer * 6.0f);
    uint8_t alpha = static_cast<uint8_t>(220 * pulse);

    // Glow halo behind sprite
    SpriteRegistry::draw(renderer, "powerup_glow",
                         sx - 3.0f, position.y - 3.0f,
                         width + 6.0f, height + 6.0f,
                         static_cast<uint8_t>(alpha / 3));

    // Powerup sprite
    SpriteRegistry::draw(renderer, powerUpSpriteName(m_type),
                         sx, position.y, 0.f, 0.f, alpha);
}

}  // namespace LightsOut
