#include "gameplay/powerups/DoubleTail.h"
#include "gameplay/Player.h"
#include "core/Constants.h"

namespace LightsOut {

DoubleTail::DoubleTail(float worldX, float worldY)
    : PowerUp(PowerUpType::DoubleTail, worldX, worldY) {
    m_color = {200, 160, 240};  // soft purple
}

void DoubleTail::apply(Player& player) {
    m_collected = true;
    player.applyDoubleTail(POWERUP_DURATION_DOUBLE);
    if (onCollect) onCollect(m_type);
}

}  // namespace LightsOut
