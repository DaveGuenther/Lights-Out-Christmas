#include <catch2/catch_all.hpp>
#include "gameplay/DarknessManager.h"
#include "core/Constants.h"

using namespace LightsOut;
using Catch::Approx;

TEST_CASE("DarknessManager: starts at zero", "[darkness]") {
    DarknessManager dm;
    REQUIRE(dm.darkness() == Approx(0.0f));
    REQUIRE_FALSE(dm.isFull());
}

TEST_CASE("DarknessManager: fills with lights", "[darkness]") {
    DarknessManager dm;
    dm.onLightOff(10);
    REQUIRE(dm.darkness() > 0.0f);
    REQUIRE(dm.darkness() == Approx(DARKNESS_FILL_PER_LIGHT * 10.0f));
}

TEST_CASE("DarknessManager: fills with house blackout", "[darkness]") {
    DarknessManager dm;
    float before = dm.darkness();
    dm.onHouseBlackout();
    REQUIRE(dm.darkness() > before);
    REQUIRE(dm.darkness() == Approx(DARKNESS_FILL_PER_HOUSE));
}

TEST_CASE("DarknessManager: clamps at 1.0", "[darkness]") {
    DarknessManager dm;
    // Fill it way past 1.0
    for (int i = 0; i < 1000; ++i) dm.onLightOff(10);
    REQUIRE(dm.darkness() == Approx(1.0f));
}

TEST_CASE("DarknessManager: callback fires when full", "[darkness]") {
    DarknessManager dm;
    bool fired = false;
    dm.onNeighborhoodDark = [&]() { fired = true; };

    // Fill just below full
    int bulbs = static_cast<int>(1.0f / DARKNESS_FILL_PER_LIGHT) - 2;
    dm.onLightOff(bulbs);
    REQUIRE_FALSE(fired);

    // Now push over the edge
    dm.onLightOff(10);
    REQUIRE(fired);
}

TEST_CASE("DarknessManager: callback fires only once", "[darkness]") {
    DarknessManager dm;
    int callCount = 0;
    dm.onNeighborhoodDark = [&]() { callCount++; };

    for (int i = 0; i < 1000; ++i) dm.onLightOff(10);
    REQUIRE(callCount == 1);
}

TEST_CASE("DarknessManager: acknowledgeBonus resets trigger flag", "[darkness]") {
    DarknessManager dm;
    dm.onNeighborhoodDark = [](){};

    for (int i = 0; i < 1000; ++i) dm.onLightOff(10);
    REQUIRE(dm.wasTriggered());

    dm.acknowledgeBonus();
    REQUIRE_FALSE(dm.wasTriggered());
}

TEST_CASE("DarknessManager: reset clears state", "[darkness]") {
    DarknessManager dm;
    dm.onLightOff(100);
    dm.reset();
    REQUIRE(dm.darkness() == Approx(0.0f));
    REQUIRE_FALSE(dm.isFull());
    REQUIRE_FALSE(dm.wasTriggered());
}
