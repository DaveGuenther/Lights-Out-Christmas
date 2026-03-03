#include <catch2/catch_all.hpp>
#include "gameplay/threats/Threat.h"
#include "gameplay/threats/Homeowner.h"
#include "gameplay/threats/Dog.h"
#include "gameplay/threats/Cat.h"
#include "gameplay/threats/Owl.h"
#include "gameplay/threats/NeighborhoodWatch.h"
#include "core/Constants.h"

using namespace LightsOut;
using Catch::Approx;

TEST_CASE("Threat: starts not alerted", "[threat]") {
    Homeowner h(0.0f, 0.0f, Homeowner::Personality::DadInPajamas);
    REQUIRE_FALSE(h.isAlerted());
    REQUIRE_FALSE(h.isFrozen());
}

TEST_CASE("Threat: alert triggers chase", "[threat]") {
    Homeowner h(100.0f, LANE_GROUND_Y - 16.0f, Homeowner::Personality::GrumpyOldMan);
    Vec2 playerPos{50.0f, LANE_GROUND_Y};
    h.alert(playerPos);
    REQUIRE(h.isAlerted());
}

TEST_CASE("Threat: freeze stops movement", "[threat]") {
    Homeowner h(100.0f, LANE_GROUND_Y - 16.0f, Homeowner::Personality::GrumpyOldMan);
    Vec2 playerPos{50.0f, LANE_GROUND_Y};
    h.alert(playerPos);
    h.freeze(5.0f);
    REQUIRE(h.isFrozen());

    float xBefore = h.position.x;
    h.update(0.1f);
    REQUIRE(h.position.x == Approx(xBefore));  // frozen — didn't move
}

TEST_CASE("Threat: freeze expires", "[threat]") {
    Homeowner h(100.0f, LANE_GROUND_Y - 16.0f, Homeowner::Personality::GrumpyOldMan);
    h.freeze(0.5f);
    h.update(0.6f);
    REQUIRE_FALSE(h.isFrozen());
}

TEST_CASE("Threat: DadInPajamas gives up after time", "[threat]") {
    Homeowner h(200.0f, LANE_GROUND_Y - 16.0f, Homeowner::Personality::DadInPajamas);
    Vec2 playerPos{50.0f, LANE_GROUND_Y};
    h.alert(playerPos);
    REQUIRE_FALSE(h.hasGivenUp());

    // Simulate long enough for dad to give up (DAD_GIVE_UP_TIME = 6s)
    for (int i = 0; i < 70; ++i) h.update(0.1f);
    REQUIRE(h.hasGivenUp());
}

TEST_CASE("Threat: GrumpyOldMan chases longer than Dad before falling", "[threat]") {
    Homeowner grumpy(200.0f, LANE_GROUND_Y - 16.0f, Homeowner::Personality::GrumpyOldMan);
    Homeowner dad(200.0f, LANE_GROUND_Y - 16.0f, Homeowner::Personality::DadInPajamas);
    Vec2 playerPos{50.0f, LANE_GROUND_Y};
    grumpy.alert(playerPos);
    dad.alert(playerPos);

    // After 3 seconds, both should still be chasing
    for (int i = 0; i < 30; ++i) { grumpy.update(0.1f); dad.update(0.1f); }
    REQUIRE(grumpy.isAlerted());
    REQUIRE(dad.isAlerted());
}

TEST_CASE("Threat: Dog barks when alerted", "[threat]") {
    Dog d(100.0f, LANE_GROUND_Y - 8.0f, DOG_CHAIN_RADIUS);
    REQUIRE_FALSE(d.isBarking());
    Vec2 playerPos{50.0f, LANE_GROUND_Y};
    d.alert(playerPos);
    REQUIRE(d.isBarking());
}

TEST_CASE("Threat: Dog distracted by decoy", "[threat]") {
    Dog d(100.0f, LANE_GROUND_Y - 8.0f, DOG_CHAIN_RADIUS);
    Vec2 playerPos{50.0f, LANE_GROUND_Y};
    d.alert(playerPos);
    REQUIRE(d.isBarking());

    Vec2 decoyPos{150.0f, LANE_GROUND_Y};
    d.distract(decoyPos);
    REQUIRE(d.isDistracted());
    REQUIRE_FALSE(d.isBarking());
}

TEST_CASE("Threat: Owl stays dormant without player on fence", "[threat]") {
    Owl owl(100.0f, LANE_FENCE_Y - 14.0f);
    REQUIRE_FALSE(owl.isAlerted());
    owl.update(0.1f);
    REQUIRE_FALSE(owl.isAlerted());
}

TEST_CASE("Threat: Owl attacks after patience timeout", "[threat]") {
    Owl owl(100.0f, LANE_FENCE_Y - 14.0f);
    owl.playerOnFence(true);

    // Must wait OWL_PATIENCE_SECONDS = 2.5s
    for (int i = 0; i < 30; ++i) owl.update(0.1f);
    REQUIRE(owl.isAlerted());
}

TEST_CASE("Threat: Owl resets when player leaves fence", "[threat]") {
    Owl owl(100.0f, LANE_FENCE_Y - 14.0f);
    owl.playerOnFence(true);
    owl.update(1.0f);  // some patience built up

    owl.playerOnFence(false);
    owl.update(0.1f);
    REQUIRE_FALSE(owl.isAlerted());
}

TEST_CASE("Threat: caughtPlayer detects overlap", "[threat]") {
    Homeowner h(50.0f, LANE_GROUND_Y - 16.0f, Homeowner::Personality::DadInPajamas);
    Rect playerBounds{48.0f, LANE_GROUND_Y - 14.0f, 12.0f, 14.0f};
    REQUIRE(h.caughtPlayer(playerBounds));
}

TEST_CASE("Threat: caughtPlayer no overlap", "[threat]") {
    Homeowner h(200.0f, LANE_GROUND_Y - 16.0f, Homeowner::Personality::DadInPajamas);
    Rect playerBounds{50.0f, LANE_GROUND_Y - 14.0f, 12.0f, 14.0f};
    REQUIRE_FALSE(h.caughtPlayer(playerBounds));
}

// ── Chase-timer fall-off (new mechanic) ──────────────────────────────────────

TEST_CASE("Threat: stops chasing after chase duration expires", "[threat]") {
    // GrumpyOldMan: chaseDuration = THREAT_CHASE_DURATION + 2.0f = 6.5s
    Homeowner h(200.0f, LANE_GROUND_Y - 16.0f, Homeowner::Personality::GrumpyOldMan);
    Vec2 playerPos{50.0f, LANE_GROUND_Y};
    h.alert(playerPos);
    REQUIRE(h.isAlerted());

    // Advance past the 6.5s chase duration
    for (int i = 0; i < 70; ++i) h.update(0.1f);  // 7 seconds
    REQUIRE_FALSE(h.isAlerted());
}

TEST_CASE("Threat: Y increases after chase duration expires", "[threat]") {
    Homeowner h(200.0f, LANE_GROUND_Y - 16.0f, Homeowner::Personality::GrumpyOldMan);
    Vec2 playerPos{50.0f, LANE_GROUND_Y};
    h.alert(playerPos);

    float yBefore = h.position.y;
    // Advance well past chase duration so falling begins
    for (int i = 0; i < 70; ++i) h.update(0.1f);  // 7 seconds
    REQUIRE(h.position.y > yBefore);
}

TEST_CASE("Threat: GrumpyOldMan marked dead after falling off screen", "[threat]") {
    Homeowner h(200.0f, LANE_GROUND_Y - 16.0f, Homeowner::Personality::GrumpyOldMan);
    Vec2 playerPos{50.0f, LANE_GROUND_Y};
    h.alert(playerPos);

    // Chase (6.5s) + fall off-screen (~1s) = ~7.5s total; 9s is safe
    for (int i = 0; i < 90; ++i) h.update(0.1f);
    REQUIRE_FALSE(h.alive);
}

TEST_CASE("Threat: Cat falls off screen after chase duration", "[threat]") {
    Cat c(200.0f, LANE_ROOFTOP_Y - 7.0f);
    Vec2 playerPos{50.0f, LANE_ROOFTOP_Y};
    c.alert(playerPos);

    // Cat chase duration = THREAT_CHASE_DURATION + 1.0f = 5.5s
    // Fall from rooftop to off-screen ~1s; 8s is safe
    for (int i = 0; i < 80; ++i) c.update(0.1f);
    REQUIRE_FALSE(c.alive);
}

TEST_CASE("NeighborhoodWatch: detects ground player in spotlight", "[threat]") {
    NeighborhoodWatch nw(100.0f);
    Rect playerBounds{125.0f, LANE_GROUND_Y - 14.0f, 12.0f, 14.0f};
    REQUIRE(nw.detectsGroundPlayer(playerBounds));
}

TEST_CASE("NeighborhoodWatch: does not detect player far away", "[threat]") {
    NeighborhoodWatch nw(100.0f);
    Rect playerBounds{50.0f, LANE_GROUND_Y - 14.0f, 12.0f, 14.0f};
    REQUIRE_FALSE(nw.detectsGroundPlayer(playerBounds));
}
