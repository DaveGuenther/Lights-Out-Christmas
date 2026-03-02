#include "gameplay/powerups/WinterCoat.h"
#include "gameplay/Player.h"
#include "core/Constants.h"

namespace LightsOut {

WinterCoat::WinterCoat(float worldX, float worldY)
    : PowerUp(PowerUpType::WinterCoat, worldX, worldY) {
    m_color = {200, 230, 255};  // icy blue-white
}

void WinterCoat::apply(Player& player) {
    m_collected = true;
    player.applyInvincibility(POWERUP_DURATION_COAT);
    if (onCollect) onCollect(m_type);
}

}  // namespace LightsOut
