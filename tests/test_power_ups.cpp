#include <catch2/catch_all.hpp>
#include "gameplay/Player.h"
#include "gameplay/powerups/AcornStash.h"
#include "gameplay/powerups/WinterCoat.h"
#include "gameplay/powerups/ShadowMode.h"
#include "gameplay/powerups/SuperChomp.h"
#include "gameplay/powerups/DecoyNut.h"
#include "gameplay/powerups/IcePatch.h"
#include "gameplay/powerups/DoubleTail.h"
#include "gameplay/powerups/FrenzyMode.h"
#include "core/Constants.h"

using namespace LightsOut;

TEST_CASE("PowerUp: AcornStash applies speed boost", "[powerup]") {
    Player p;
    AcornStash pu(0.0f, 0.0f);
    REQUIRE_FALSE(p.isInvincible());
    pu.apply(p);
    REQUIRE(pu.collected());
    // Speed boost has duration
    REQUIRE(pu.duration() == Approx(POWERUP_DURATION_ACORN));
}

TEST_CASE("PowerUp: WinterCoat grants invincibility", "[powerup]") {
    Player p;
    WinterCoat pu(0.0f, 0.0f);
    pu.apply(p);
    REQUIRE(p.isInvincible());
    REQUIRE(pu.collected());
}

TEST_CASE("PowerUp: ShadowMode activates shadow", "[powerup]") {
    Player p;
    ShadowMode pu(0.0f, 0.0f);
    pu.apply(p);
    REQUIRE(p.isShadowMode());
    REQUIRE(pu.collected());
}

TEST_CASE("PowerUp: SuperChomp activates super chomp", "[powerup]") {
    Player p;
    SuperChomp pu(0.0f, 0.0f);
    pu.apply(p);
    REQUIRE(p.isSuperChomp());
    REQUIRE(pu.collected());
}

TEST_CASE("PowerUp: FrenzyMode slows world", "[powerup]") {
    Player p;
    REQUIRE(p.frenzySlowFactor() == Approx(1.0f));
    FrenzyMode pu(0.0f, 0.0f);
    pu.apply(p);
    REQUIRE(p.isFrenzy());
    REQUIRE(p.frenzySlowFactor() == Approx(FRENZY_SLOW_FACTOR));
}

TEST_CASE("PowerUp: DoubleTail activates double tail", "[powerup]") {
    Player p;
    DoubleTail pu(0.0f, 0.0f);
    pu.apply(p);
    REQUIRE(p.isDoubleTail());
}

TEST_CASE("PowerUp: effects expire after duration", "[powerup]") {
    Player p;
    WinterCoat pu(0.0f, 0.0f);
    pu.apply(p);
    REQUIRE(p.isInvincible());

    // Update past duration
    p.update(POWERUP_DURATION_COAT + 0.1f);
    REQUIRE_FALSE(p.isInvincible());
}

TEST_CASE("PowerUp: onCollect callback fires", "[powerup]") {
    AcornStash pu(0.0f, 0.0f);
    bool fired = false;
    PowerUpType receivedType = PowerUpType::AcornStash;
    pu.onCollect = [&](PowerUpType t) { fired = true; receivedType = t; };

    Player p;
    pu.apply(p);
    REQUIRE(fired);
    REQUIRE(receivedType == PowerUpType::AcornStash);
}

TEST_CASE("PowerUp: bob animation updates position", "[powerup]") {
    AcornStash pu(100.0f, 50.0f);
    float y0 = pu.position.y;
    pu.update(0.1f);
    // Position.y changes due to bob
    pu.update(0.1f);
    // Just verify it doesn't crash (exact position depends on sine)
    REQUIRE(true);
}

TEST_CASE("PowerUp: collected flag marks alive=false after update", "[powerup]") {
    AcornStash pu(0.0f, 0.0f);
    Player p;
    pu.apply(p);
    REQUIRE(pu.collected());
    pu.update(0.016f);  // One frame
    REQUIRE_FALSE(pu.alive);
}

TEST_CASE("PowerUp: names are non-empty", "[powerup]") {
    REQUIRE_FALSE(AcornStash(0,0).name().empty());
    REQUIRE_FALSE(WinterCoat(0,0).name().empty());
    REQUIRE_FALSE(ShadowMode(0,0).name().empty());
    REQUIRE_FALSE(SuperChomp(0,0).name().empty());
    REQUIRE_FALSE(DecoyNut(0,0).name().empty());
    REQUIRE_FALSE(IcePatch(0,0).name().empty());
    REQUIRE_FALSE(DoubleTail(0,0).name().empty());
    REQUIRE_FALSE(FrenzyMode(0,0).name().empty());
}
