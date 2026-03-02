#include "gameplay/DarknessManager.h"
#include "core/Constants.h"
#include <algorithm>

namespace LightsOut {

void DarknessManager::onLightOff(int bulbCount) {
    m_darkness += DARKNESS_FILL_PER_LIGHT * static_cast<float>(bulbCount);
    m_darkness  = std::min(m_darkness, 1.0f);
    if (m_darkness >= 1.0f && !m_triggered) {
        m_triggered = true;
        if (onNeighborhoodDark) onNeighborhoodDark();
    }
}

void DarknessManager::onHouseBlackout() {
    m_darkness += DARKNESS_FILL_PER_HOUSE;
    m_darkness  = std::min(m_darkness, 1.0f);
    if (m_darkness >= 1.0f && !m_triggered) {
        m_triggered = true;
        if (onNeighborhoodDark) onNeighborhoodDark();
    }
}

void DarknessManager::reset() {
    m_darkness  = 0.0f;
    m_triggered = false;
}

}  // namespace LightsOut
