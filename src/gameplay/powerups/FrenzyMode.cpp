#include "gameplay/powerups/FrenzyMode.h"
#include "gameplay/Player.h"
#include "core/Constants.h"

namespace LightsOut {

FrenzyMode::FrenzyMode(float worldX, float worldY)
    : PowerUp(PowerUpType::FrenzyMode, worldX, worldY) {
    m_color = {200, 50, 200};  // vivid magenta
}

void FrenzyMode::apply(Player& player) {
    m_collected = true;
    player.applyFrenzyMode(POWERUP_DURATION_FRENZY);
    if (onCollect) onCollect(m_type);
}

}  // namespace LightsOut
