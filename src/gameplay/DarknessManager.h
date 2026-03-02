#pragma once
#include <functional>

namespace LightsOut {

// Tracks the "neighborhood darkness meter" — fills as you kill lights and
// triggers a stage bonus when full.
class DarknessManager {
public:
    DarknessManager() = default;

    void onLightOff(int bulbCount);
    void onHouseBlackout();
    void reset();

    float darkness() const { return m_darkness; }   // 0.0 – 1.0
    bool  isFull()   const { return m_darkness >= 1.0f; }
    bool  wasTriggered() const { return m_triggered; }

    void acknowledgeBonus() { m_triggered = false; }   // call after awarding bonus

    std::function<void()> onNeighborhoodDark;  // fired when meter reaches 1.0

private:
    float m_darkness  = 0.0f;
    bool  m_triggered = false;
};

}  // namespace LightsOut
