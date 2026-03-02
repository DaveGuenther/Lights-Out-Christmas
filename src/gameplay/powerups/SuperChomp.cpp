#include "gameplay/powerups/SuperChomp.h"
#include "gameplay/Player.h"
#include "core/Constants.h"

namespace LightsOut {

SuperChomp::SuperChomp(float worldX, float worldY)
    : PowerUp(PowerUpType::SuperChomp, worldX, worldY) {
    m_color = {220, 60, 60};  // bright red
}

void SuperChomp::apply(Player& player) {
    m_collected = true;
    player.applySuperChomp(POWERUP_DURATION_CHOMP);
    if (onCollect) onCollect(m_type);
}

}  // namespace LightsOut
