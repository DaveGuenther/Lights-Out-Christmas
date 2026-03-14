#include <catch2/catch_all.hpp>
#include "gameplay/House.h"
#include "core/Constants.h"
#include "core/HouseAssetLoader.h"

using namespace LightsOut;

// A minimal asset with empty sprite/mask paths.
// IMG_Load("") will fail gracefully, so parseMask returns empty mask data —
// meaning no light strings are placed. This lets us test the House API without
// needing real image files in the test environment.
static HouseAsset makeFallbackAsset(float width = 256.0f) {
    return HouseAsset{"test_house", "", "", width};
}

// ── Basic construction ────────────────────────────────────────────────────────

TEST_CASE("House: dark house (no tiers) has no light strings", "[house]") {
    auto asset = makeFallbackAsset();
    House h(0.0f, HouseStyle::Simple, 0, asset,
            false, false, false, 1, 0.0f);
    REQUIRE(h.lightStrings().empty());
}

TEST_CASE("House: sprite width matches asset pixel width", "[house]") {
    auto asset256 = makeFallbackAsset(256.0f);
    House h256(0.0f, HouseStyle::Simple, 0, asset256,
               false, false, false, 1, 0.0f);
    REQUIRE(h256.spriteWidth() == Catch::Approx(256.0f));

    auto asset320 = makeFallbackAsset(320.0f);
    House h320(0.0f, HouseStyle::Simple, 1, asset320,
               false, false, false, 1, 0.0f);
    REQUIRE(h320.spriteWidth() == Catch::Approx(320.0f));
}

TEST_CASE("House: houseIndex is stored correctly", "[house]") {
    auto asset = makeFallbackAsset();
    House h(0.0f, HouseStyle::Simple, 42, asset,
            false, false, false, 1, 0.0f);
    REQUIRE(h.houseIndex() == 42);
}

TEST_CASE("House: world X position is set from constructor", "[house]") {
    auto asset = makeFallbackAsset(256.0f);
    House h(100.0f, HouseStyle::Simple, 0, asset,
            false, false, false, 1, 0.0f);
    REQUIRE(h.position.x == Catch::Approx(100.0f));
}

TEST_CASE("House: bottom of house aligns with HOUSE_GROUND_Y", "[house]") {
    auto asset = makeFallbackAsset();
    House h(0.0f, HouseStyle::Simple, 0, asset,
            false, false, false, 1, 0.0f);
    float bottom = h.position.y + h.height;
    REQUIRE(bottom == Catch::Approx(HOUSE_GROUND_Y));
}

// ── isFullyDark ───────────────────────────────────────────────────────────────

TEST_CASE("House: isFullyDark is false for unlit house with no strands", "[house]") {
    auto asset = makeFallbackAsset();
    House h(0.0f, HouseStyle::Simple, 0, asset,
            false, false, false, 1, 0.0f);
    REQUIRE_FALSE(h.isFullyDark());
}

// ── Porch light ───────────────────────────────────────────────────────────────

TEST_CASE("House: porch light starts off", "[house]") {
    auto asset = makeFallbackAsset();
    House h(0.0f, HouseStyle::Simple, 0, asset,
            false, false, false, 1, 0.0f);
    REQUIRE_FALSE(h.porchLightOn());
}

TEST_CASE("House: porch light turns on when triggered", "[house]") {
    auto asset = makeFallbackAsset();
    House h(0.0f, HouseStyle::Simple, 0, asset,
            false, false, false, 1, 0.0f);
    h.triggerPorchLight();
    REQUIRE(h.porchLightOn());
}

TEST_CASE("House: porch light resets after delay", "[house]") {
    auto asset = makeFallbackAsset();
    House h(0.0f, HouseStyle::Simple, 0, asset,
            false, false, false, 1, 0.0f);
    h.triggerPorchLight();
    h.resetPorchLight(0.2f);
    h.update(0.3f);
    REQUIRE_FALSE(h.porchLightOn());
}

TEST_CASE("House: porch light does not reset before delay elapses", "[house]") {
    auto asset = makeFallbackAsset();
    House h(0.0f, HouseStyle::Simple, 0, asset,
            false, false, false, 1, 0.0f);
    h.triggerPorchLight();
    h.resetPorchLight(0.5f);
    h.update(0.1f);
    REQUIRE(h.porchLightOn());
}
