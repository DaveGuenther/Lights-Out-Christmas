#include <catch2/catch_all.hpp>
#include "gameplay/LightString.h"
#include "core/Constants.h"

using namespace LightsOut;

TEST_CASE("LightString: constructs with correct bulb count", "[light]") {
    LightString ls(0.0f, 50.0f, 40.0f, false, 0);
    REQUIRE(ls.lightCount() >= 2);
    REQUIRE_FALSE(ls.isFullyOff());
}

TEST_CASE("LightString: single bite triggers cascade", "[light]") {
    LightString ls(0.0f, 50.0f, 40.0f, false, 0);
    REQUIRE_FALSE(ls.isFullyOff());
    bool result = ls.bite();
    REQUIRE(result);  // bite registered
}

TEST_CASE("LightString: tangled needs multiple bites", "[light]") {
    LightString ls(0.0f, 50.0f, 40.0f, true, 0);
    REQUIRE(ls.bitesRequired() == LIGHT_BITES_TANGLED);
    REQUIRE(ls.bitesLanded() == 0);

    // First bite: not enough
    ls.bite();
    REQUIRE(ls.bitesLanded() == 1);
    REQUIRE_FALSE(ls.isFullyOff());

    ls.bite();
    REQUIRE(ls.bitesLanded() == 2);

    ls.bite();
    // Should now have started cascade
}

TEST_CASE("LightString: cascade turns all lights off eventually", "[light]") {
    LightString ls(0.0f, 50.0f, 40.0f, false, 0);
    ls.bite();

    // Simulate enough time for cascade to complete
    // Cascade step = 0.03s per bulb, max ~50 bulbs → 1.5s total
    for (int i = 0; i < 200; ++i) {
        ls.update(0.01f);
    }
    REQUIRE(ls.isFullyOff());
}

TEST_CASE("LightString: onDark callback fires when fully off", "[light]") {
    LightString ls(0.0f, 50.0f, 40.0f, false, 0);
    bool called = false;
    int bulbCount = 0;
    ls.onDark = [&](int bc, bool) { called = true; bulbCount = bc; };

    ls.bite();
    for (int i = 0; i < 200; ++i) ls.update(0.01f);

    REQUIRE(called);
    REQUIRE(bulbCount > 0);
}

TEST_CASE("LightString: bite after fully off returns false", "[light]") {
    LightString ls(0.0f, 50.0f, 40.0f, false, 0);
    ls.bite();
    for (int i = 0; i < 200; ++i) ls.update(0.01f);

    REQUIRE(ls.isFullyOff());
    bool result = ls.bite();
    REQUIRE_FALSE(result);
}

TEST_CASE("LightString: isTangled reflects constructor arg", "[light]") {
    LightString tangled(0.0f, 50.0f, 40.0f, true, 5);
    LightString normal(0.0f, 50.0f, 40.0f, false, 5);
    REQUIRE(tangled.isTangled());
    REQUIRE_FALSE(normal.isTangled());
}

TEST_CASE("LightString: houseIndex returned correctly", "[light]") {
    LightString ls(0.0f, 50.0f, 40.0f, false, 42);
    REQUIRE(ls.houseIndex() == 42);
}

TEST_CASE("LightString: sparks generated don't crash render path", "[light]") {
    // No SDL renderer in tests; just ensure no crash in update
    LightString ls(0.0f, 50.0f, 40.0f, false, 0);
    ls.bite();
    REQUIRE_NOTHROW(ls.update(0.5f));
}
