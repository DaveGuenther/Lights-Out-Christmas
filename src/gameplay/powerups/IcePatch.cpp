#include "gameplay/powerups/IcePatch.h"
#include "gameplay/Player.h"
#include "core/Constants.h"

namespace LightsOut {

IcePatch::IcePatch(float worldX, float worldY)
    : PowerUp(PowerUpType::IcePatch, worldX, worldY) {
    m_color = {140, 200, 240};  // ice blue
}

void IcePatch::apply(Player& /*player*/) {
    // IcePatch is applied to threats by GameWorld (freezes nearest threat);
    // the player just triggers the effect via UsePowerUp.
    m_collected = true;
    if (onCollect) onCollect(m_type);
}

}  // namespace LightsOut
