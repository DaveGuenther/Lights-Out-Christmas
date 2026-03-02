#include <catch2/catch_all.hpp>
#include "gameplay/ScoreSystem.h"
#include "core/Constants.h"

using namespace LightsOut;

TEST_CASE("ScoreSystem: starts at zero", "[score]") {
    ScoreSystem s;
    REQUIRE(s.score() == 0);
    REQUIRE(s.levelScore() == 0);
    REQUIRE(s.comboMultiplier() == Approx(1.0f));
    REQUIRE(s.comboCount() == 0);
}

TEST_CASE("ScoreSystem: lights off adds points", "[score]") {
    ScoreSystem s;
    s.onLightOff(1);
    REQUIRE(s.score() == POINTS_PER_LIGHT);
    REQUIRE(s.levelScore() == POINTS_PER_LIGHT);
}

TEST_CASE("ScoreSystem: multiple lights multiply", "[score]") {
    ScoreSystem s;
    s.onLightOff(5);
    REQUIRE(s.score() == POINTS_PER_LIGHT * 5);
}

TEST_CASE("ScoreSystem: combo increases with each event", "[score]") {
    ScoreSystem s;
    s.onLightOff(1);
    float c1 = s.comboMultiplier();
    s.onLightOff(1);
    float c2 = s.comboMultiplier();
    REQUIRE(c2 > c1);
    REQUIRE(c2 == Approx(c1 + ScoreSystem::COMBO_INCREMENT));
}

TEST_CASE("ScoreSystem: combo resets after timeout", "[score]") {
    ScoreSystem s;
    s.onLightOff(1);
    s.onLightOff(1);
    REQUIRE(s.comboCount() == 2);

    // Simulate time passing beyond timeout
    s.update(ScoreSystem::COMBO_TIMEOUT + 0.1f);
    REQUIRE(s.comboMultiplier() == Approx(1.0f));
    REQUIRE(s.comboCount() == 0);
}

TEST_CASE("ScoreSystem: combo doesn't reset before timeout", "[score]") {
    ScoreSystem s;
    s.onLightOff(1);
    s.update(ScoreSystem::COMBO_TIMEOUT * 0.5f);
    REQUIRE(s.comboCount() >= 1);
}

TEST_CASE("ScoreSystem: house blackout adds bonus", "[score]") {
    ScoreSystem s;
    int before = s.score();
    s.onHouseBlackout(0);
    REQUIRE(s.score() > before + FULL_HOUSE_BONUS - 1);
}

TEST_CASE("ScoreSystem: combo capped at MAX_COMBO", "[score]") {
    ScoreSystem s;
    for (int i = 0; i < 50; ++i) s.onLightOff(1);
    REQUIRE(s.comboMultiplier() <= ScoreSystem::MAX_COMBO);
}

TEST_CASE("ScoreSystem: reset clears everything", "[score]") {
    ScoreSystem s;
    s.onLightOff(10);
    s.onHouseBlackout(0);
    s.reset();
    REQUIRE(s.score() == 0);
    REQUIRE(s.levelScore() == 0);
    REQUIRE(s.comboMultiplier() == Approx(1.0f));
}

TEST_CASE("ScoreSystem: resetLevel clears level score but not total", "[score]") {
    ScoreSystem s;
    s.onLightOff(10);
    int total = s.score();
    s.resetLevel();
    REQUIRE(s.levelScore() == 0);
    REQUIRE(s.score() == total);
}

TEST_CASE("ScoreSystem: chain reaction adds bonus", "[score]") {
    ScoreSystem s;
    int before = s.score();
    s.onChainReaction(3);
    REQUIRE(s.score() > before);
    REQUIRE(s.score() >= before + CHAIN_REACTION_BONUS * 3);
}

TEST_CASE("ScoreSystem: onScore callback fires", "[score]") {
    ScoreSystem s;
    int callCount = 0;
    s.onScore = [&](const ScoreEvent&) { callCount++; };
    s.onLightOff(1);
    REQUIRE(callCount == 1);
    s.onHouseBlackout(0);
    REQUIRE(callCount == 2);
}

TEST_CASE("ScoreSystem: high score tracks maximum", "[score]") {
    ScoreSystem s;
    s.onLightOff(100);
    int peak = s.score();
    s.reset();
    REQUIRE(s.highScore() == peak);
    // After reset and low score, high score stays
    s.onLightOff(1);
    REQUIRE(s.highScore() == peak);
}
