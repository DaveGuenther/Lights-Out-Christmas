#include "gameplay/powerups/ShadowMode.h"
#include "gameplay/Player.h"
#include "core/Constants.h"

namespace LightsOut {

ShadowMode::ShadowMode(float worldX, float worldY)
    : PowerUp(PowerUpType::ShadowMode, worldX, worldY) {
    m_color = {60, 60, 120};  // dark indigo
}

void ShadowMode::apply(Player& player) {
    m_collected = true;
    player.applyShadowMode(POWERUP_DURATION_SHADOW);
    if (onCollect) onCollect(m_type);
}

}  // namespace LightsOut
