#include "gameplay/ScoreSystem.h"
#include "core/Constants.h"
#include <algorithm>
#include <cmath>

namespace LightsOut {

void ScoreSystem::onLightOff(int bulbCount) {
    int base = POINTS_PER_LIGHT * bulbCount;
    addPoints(base, false, false);
    incrementCombo();
    m_comboTimer = COMBO_TIMEOUT;
}

void ScoreSystem::onHouseBlackout(int /*houseIndex*/) {
    int base = FULL_HOUSE_BONUS;
    addPoints(base, true, false);
    // House blackout gives a large combo bump
    m_combo = std::min(m_combo * static_cast<float>(FULL_HOUSE_MULTIPLIER) * 0.5f, MAX_COMBO);
    m_comboTimer = COMBO_TIMEOUT;
}

void ScoreSystem::onChainReaction(int strandsKilled) {
    int base = CHAIN_REACTION_BONUS * strandsKilled;
    addPoints(base, false, true);
    incrementCombo();
    m_comboTimer = COMBO_TIMEOUT;
}

void ScoreSystem::update(float dt) {
    if (m_combo > 1.0f) {
        m_comboTimer -= dt;
        if (m_comboTimer <= 0.0f) {
            m_combo      = 1.0f;
            m_comboCount = 0;
            m_comboTimer = 0.0f;
        }
    }
}

void ScoreSystem::resetLevel() {
    m_levelScore = 0;
}

void ScoreSystem::reset() {
    m_score      = 0;
    m_levelScore = 0;
    m_combo      = 1.0f;
    m_comboCount = 0;
    m_comboTimer = 0.0f;
    // Keep high score across resets
}

void ScoreSystem::addPoints(int base, bool fullHouse, bool chain) {
    int final_ = static_cast<int>(static_cast<float>(base) * m_combo);
    if (fullHouse) final_ = static_cast<int>(final_ * FULL_HOUSE_MULTIPLIER);

    m_score      += final_;
    m_levelScore += final_;
    if (m_score > m_highScore) m_highScore = m_score;

    if (onScore) {
        ScoreEvent evt;
        evt.points             = final_;
        evt.comboMultiplier    = m_combo;
        evt.isFullHouseBlackout = fullHouse;
        evt.isChainReaction    = chain;
        onScore(evt);
    }
}

void ScoreSystem::incrementCombo() {
    m_comboCount++;
    m_combo = std::min(1.0f + static_cast<float>(m_comboCount) * COMBO_INCREMENT, MAX_COMBO);
}

}  // namespace LightsOut
