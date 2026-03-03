#include <catch2/catch_all.hpp>
#include "gameplay/House.h"
#include "core/Constants.h"

using namespace LightsOut;
using Catch::Approx;

// ── Strand counts by difficulty tier ─────────────────────────────────────────

TEST_CASE("House: dark house has no light strings", "[house]") {
    House h(0.0f, 60.0f, HouseStyle::Simple, 0, 0, 0, 0, 0.0f);
    REQUIRE(h.lightStrings().empty());
}

TEST_CASE("House: easy house with roof only has one strand", "[house]") {
    House h(0.0f, 60.0f, HouseStyle::Simple, 0, 1, 0, 0, 0.0f);
    REQUIRE(h.lightStrings().size() == 1);
}

TEST_CASE("House: easy house with window only has one strand", "[house]") {
    House h(0.0f, 60.0f, HouseStyle::Simple, 0, 0, 1, 0, 0.0f);
    REQUIRE(h.lightStrings().size() == 1);
}

TEST_CASE("House: easy house with porch only has one strand", "[house]") {
    House h(0.0f, 60.0f, HouseStyle::Simple, 0, 0, 0, 1, 0.0f);
    REQUIRE(h.lightStrings().size() == 1);
}

TEST_CASE("House: medium house with two tiers has two strands", "[house]") {
    House roofWin(0.0f, 60.0f, HouseStyle::Simple, 0, 1, 1, 0, 0.0f);
    House winPorch(0.0f, 60.0f, HouseStyle::Simple, 1, 0, 1, 1, 0.0f);
    REQUIRE(roofWin.lightStrings().size()  == 2);
    REQUIRE(winPorch.lightStrings().size() == 2);
}

TEST_CASE("House: hard house with all three tiers has three strands", "[house]") {
    House h(0.0f, 60.0f, HouseStyle::Simple, 0, 1, 1, 1, 0.0f);
    REQUIRE(h.lightStrings().size() == 3);
}

TEST_CASE("House: multiple strands per tier multiply total count", "[house]") {
    // 2 strands per tier × 3 tiers = 6 total
    House h(0.0f, 60.0f, HouseStyle::Simple, 0, 2, 2, 2, 0.0f);
    REQUIRE(h.lightStrings().size() == 6);
}

// ── Light tier Y positions ────────────────────────────────────────────────────

TEST_CASE("House: roof light Y is near roofline", "[house]") {
    House h(0.0f, 60.0f, HouseStyle::Simple, 0, 1, 0, 0, 0.0f);
    REQUIRE_FALSE(h.lightStrings().empty());
    float expectedY = HOUSE_GROUND_Y - HOUSE_HEIGHT + 2.0f;  // position.y + 2
    REQUIRE(h.lightStrings()[0]->position.y == Approx(expectedY));
}

TEST_CASE("House: window light Y is below roof light Y", "[house]") {
    House roofHouse(0.0f, 60.0f, HouseStyle::Simple, 0, 1, 0, 0, 0.0f);
    House winHouse( 0.0f, 60.0f, HouseStyle::Simple, 1, 0, 1, 0, 0.0f);
    float roofY = roofHouse.lightStrings()[0]->position.y;
    float winY  = winHouse.lightStrings()[0]->position.y;
    REQUIRE(winY > roofY);
}

TEST_CASE("House: porch light Y is the lowest tier", "[house]") {
    House winHouse(0.0f, 60.0f, HouseStyle::Simple, 0, 0, 1, 0, 0.0f);
    House porchHouse(0.0f, 60.0f, HouseStyle::Simple, 1, 0, 0, 1, 0.0f);
    float winY   = winHouse.lightStrings()[0]->position.y;
    float porchY = porchHouse.lightStrings()[0]->position.y;
    REQUIRE(porchY > winY);
    // Porch lights must sit below the window ledge but above the ground
    REQUIRE(porchY < LANE_GROUND_Y);
}

// ── isFullyDark ───────────────────────────────────────────────────────────────

TEST_CASE("House: isFullyDark is false when lights are on", "[house]") {
    House h(0.0f, 60.0f, HouseStyle::Simple, 0, 1, 0, 0, 0.0f);
    REQUIRE_FALSE(h.isFullyDark());
}

TEST_CASE("House: isFullyDark is false for a dark house with no strands", "[house]") {
    // A never-lit house doesn't count as a completed blackout
    House h(0.0f, 60.0f, HouseStyle::Simple, 0, 0, 0, 0, 0.0f);
    REQUIRE_FALSE(h.isFullyDark());
}

// ── Porch light ───────────────────────────────────────────────────────────────

TEST_CASE("House: porch light starts off", "[house]") {
    House h(0.0f, 60.0f, HouseStyle::Simple, 0, 0, 0, 0, 0.0f);
    REQUIRE_FALSE(h.porchLightOn());
}

TEST_CASE("House: porch light turns on when triggered", "[house]") {
    House h(0.0f, 60.0f, HouseStyle::Simple, 0, 0, 0, 0, 0.0f);
    h.triggerPorchLight();
    REQUIRE(h.porchLightOn());
}

TEST_CASE("House: porch light resets after delay", "[house]") {
    House h(0.0f, 60.0f, HouseStyle::Simple, 0, 0, 0, 0, 0.0f);
    h.triggerPorchLight();
    h.resetPorchLight(0.2f);
    h.update(0.3f);  // advance past delay
    REQUIRE_FALSE(h.porchLightOn());
}
