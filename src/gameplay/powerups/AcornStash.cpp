#include "gameplay/powerups/AcornStash.h"
#include "gameplay/Player.h"
#include "core/Constants.h"

namespace LightsOut {

AcornStash::AcornStash(float worldX, float worldY)
    : PowerUp(PowerUpType::AcornStash, worldX, worldY) {
    m_color = {180, 120, 40};  // acorn brown/gold
}

void AcornStash::apply(Player& player) {
    m_collected = true;
    player.applySpeedBoost(ACORN_SPEED_MULTIPLIER, POWERUP_DURATION_ACORN);
    if (onCollect) onCollect(m_type);
}

}  // namespace LightsOut
