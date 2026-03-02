#pragma once
#include <functional>
#include <string>

namespace LightsOut {

struct ScoreEvent {
    int   points;
    float comboMultiplier;
    bool  isFullHouseBlackout;
    bool  isChainReaction;
};

class ScoreSystem {
public:
    ScoreSystem() = default;

    // Called when a light bulb goes dark
    void onLightOff(int bulbCount);

    // Called when an entire house goes dark (triggers multiplier)
    void onHouseBlackout(int houseIndex);

    // Called when a chain reaction kills multiple strands at once
    void onChainReaction(int strandsKilled);

    // Called each frame to update speed-bonus tracking
    void update(float dt);

    // Reset for a new level
    void resetLevel();

    // Reset everything (new game)
    void reset();

    int   score()           const { return m_score; }
    int   levelScore()      const { return m_levelScore; }
    float comboMultiplier() const { return m_combo; }
    int   comboCount()      const { return m_comboCount; }
    int   highScore()       const { return m_highScore; }

    // Combo expires after this many seconds without scoring
    static constexpr float COMBO_TIMEOUT = 2.0f;
    static constexpr float COMBO_INCREMENT = 0.5f;
    static constexpr float MAX_COMBO = 8.0f;

    // Callback fired after each scoring event (for HUD flash)
    std::function<void(const ScoreEvent&)> onScore;

private:
    int   m_score      = 0;
    int   m_levelScore = 0;
    int   m_highScore  = 0;
    float m_combo      = 1.0f;
    int   m_comboCount = 0;
    float m_comboTimer = 0.0f;   // counts down; resets on score event

    void addPoints(int base, bool fullHouse, bool chain);
    void incrementCombo();
};

}  // namespace LightsOut
