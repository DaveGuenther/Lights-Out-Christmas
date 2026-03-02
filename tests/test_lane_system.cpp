#include <catch2/catch_all.hpp>
#include "gameplay/Lane.h"
#include "core/Constants.h"

using namespace LightsOut;

TEST_CASE("Lane: Y positions match constants", "[lane]") {
    REQUIRE(laneY(LaneType::Rooftop)   == Approx(LANE_ROOFTOP_Y));
    REQUIRE(laneY(LaneType::PowerLine) == Approx(LANE_POWERLINE_Y));
    REQUIRE(laneY(LaneType::Branch)    == Approx(LANE_BRANCH_Y));
    REQUIRE(laneY(LaneType::Fence)     == Approx(LANE_FENCE_Y));
    REQUIRE(laneY(LaneType::Ground)    == Approx(LANE_GROUND_Y));
}

TEST_CASE("Lane: laneAbove returns correct lane", "[lane]") {
    REQUIRE(laneAbove(LaneType::Ground)    == LaneType::Fence);
    REQUIRE(laneAbove(LaneType::Fence)     == LaneType::Branch);
    REQUIRE(laneAbove(LaneType::Branch)    == LaneType::PowerLine);
    REQUIRE(laneAbove(LaneType::PowerLine) == LaneType::Rooftop);
    REQUIRE(laneAbove(LaneType::Rooftop)   == LaneType::Rooftop);  // stays at top
}

TEST_CASE("Lane: laneBelow returns correct lane", "[lane]") {
    REQUIRE(laneBelow(LaneType::Rooftop)   == LaneType::PowerLine);
    REQUIRE(laneBelow(LaneType::PowerLine) == LaneType::Branch);
    REQUIRE(laneBelow(LaneType::Branch)    == LaneType::Fence);
    REQUIRE(laneBelow(LaneType::Fence)     == LaneType::Ground);
    REQUIRE(laneBelow(LaneType::Ground)    == LaneType::Ground);  // stays at bottom
}

TEST_CASE("Lane: laneIndex and laneFromIndex are inverse", "[lane]") {
    for (int i = 0; i < NUM_LANES; ++i) {
        LaneType lt = laneFromIndex(i);
        REQUIRE(laneIndex(lt) == i);
    }
}

TEST_CASE("Lane: isOnLane detects correct position", "[lane]") {
    float y = LANE_GROUND_Y;
    REQUIRE(isOnLane(LaneType::Ground, y));
    REQUIRE_FALSE(isOnLane(LaneType::Fence, y));
}

TEST_CASE("Lane: isOnLane respects tolerance", "[lane]") {
    float y = LANE_GROUND_Y + 3.0f;
    REQUIRE(isOnLane(LaneType::Ground, y, 4.0f));
    REQUIRE_FALSE(isOnLane(LaneType::Ground, y, 2.0f));
}

TEST_CASE("Lane: laneInfo returns correct attributes", "[lane]") {
    LaneInfo groundInfo = laneInfo(LaneType::Ground);
    REQUIRE(groundInfo.hasHomeowners == true);
    REQUIRE(groundInfo.hasCats == false);
    REQUIRE(groundInfo.hasOwl == false);

    LaneInfo branchInfo = laneInfo(LaneType::Branch);
    REQUIRE(branchInfo.hasOwl == true);
    REQUIRE(branchInfo.hasCats == true);
    REQUIRE(branchInfo.hasHomeowners == false);

    LaneInfo rooftopInfo = laneInfo(LaneType::Rooftop);
    REQUIRE(rooftopInfo.slippery == true);
    REQUIRE(rooftopInfo.hasCats == true);
}

TEST_CASE("Lane: laneAbove/laneBelow are inverse for middle lanes", "[lane]") {
    for (int i = 1; i < NUM_LANES - 1; ++i) {
        LaneType lt = laneFromIndex(i);
        REQUIRE(laneBelow(laneAbove(lt)) == lt);
        REQUIRE(laneAbove(laneBelow(lt)) == lt);
    }
}
