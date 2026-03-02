#include "gameplay/powerups/DecoyNut.h"
#include "gameplay/Player.h"

namespace LightsOut {

DecoyNut::DecoyNut(float worldX, float worldY)
    : PowerUp(PowerUpType::DecoyNut, worldX, worldY) {
    m_color = {160, 100, 30};  // nut brown
}

void DecoyNut::apply(Player& /*player*/) {
    // Decoy effect is handled by GameWorld when UsePowerUp is triggered;
    // it places a decoy entity and distracts nearby threats.
    m_collected = true;
    if (onCollect) onCollect(m_type);
}

}  // namespace LightsOut
