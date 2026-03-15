#include <catch2/catch_all.hpp>
#include "gameplay/BushTree.h"
#include "core/Constants.h"

using namespace LightsOut;
using Catch::Approx;

// Build a minimal BushAsset without any real file paths so the
// constructor skips IMG_Load / parsePlatforms gracefully.
static BushAsset makeFakeAsset(float w = 40.0f, float h = 30.0f,
                                const std::string& name = "test_bush") {
    BushAsset a;
    a.name          = name;
    a.spritePath    = "";   // empty → SpriteRegistry::draw is a no-op
    a.maskPath      = "";   // empty → no light strands created
    a.collisionPath = "";   // empty → no platforms
    a.pixelWidth    = w;
    a.pixelHeight   = h;
    return a;
}

TEST_CASE("BushTree: position.x matches worldX passed to constructor", "[bushtree]") {
    BushAsset asset = makeFakeAsset();
    float worldX = 350.0f;
    BushTree bt(worldX, asset, 0.0f, 0);
    REQUIRE(bt.position.x == Approx(worldX));
}

TEST_CASE("BushTree: bottom of sprite sits at LANE_GROUND_Y", "[bushtree]") {
    BushAsset asset = makeFakeAsset(40.0f, 30.0f);
    BushTree bt(100.0f, asset, 0.0f, 0);
    REQUIRE(bt.position.y + bt.height == Approx(LANE_GROUND_Y));
}

TEST_CASE("BushTree: width and height match asset pixel dimensions", "[bushtree]") {
    BushAsset asset = makeFakeAsset(46.0f, 37.0f);
    BushTree bt(0.0f, asset, 0.0f, 0);
    REQUIRE(bt.width  == Approx(46.0f));
    REQUIRE(bt.height == Approx(37.0f));
}

TEST_CASE("BushTree: no light strings when maskPath is empty", "[bushtree]") {
    BushAsset asset = makeFakeAsset();
    BushTree bt(100.0f, asset, 0.0f, 0);
    REQUIRE(bt.lightStrings().empty());
}

TEST_CASE("BushTree: no platforms when collisionPath is empty", "[bushtree]") {
    BushAsset asset = makeFakeAsset();
    BushTree bt(100.0f, asset, 0.0f, 0);
    REQUIRE(bt.platforms().empty());
}

TEST_CASE("BushTree: objectIndex returns value passed at construction", "[bushtree]") {
    BushAsset asset = makeFakeAsset();
    BushTree bt(100.0f, asset, 0.0f, 42);
    REQUIRE(bt.objectIndex() == 42);
}

TEST_CASE("BushTree: position.y is LANE_GROUND_Y minus asset height", "[bushtree]") {
    float h = 35.0f;
    BushAsset asset = makeFakeAsset(50.0f, h);
    BushTree bt(0.0f, asset, 0.0f, 0);
    REQUIRE(bt.position.y == Approx(LANE_GROUND_Y - h));
}
