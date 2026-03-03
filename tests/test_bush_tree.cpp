#include <catch2/catch_all.hpp>
#include "gameplay/BushTree.h"
#include "core/Constants.h"

using namespace LightsOut;
using Catch::Approx;

TEST_CASE("BushTree: no lights when lightCount is 0", "[bushtree]") {
    BushTree bt(100.0f, BushTree::Type::Bush, 0, 0.0f, 0);
    REQUIRE(bt.lightStrings().empty());
}

TEST_CASE("BushTree: creates correct number of light strings", "[bushtree]") {
    BushTree bt1(100.0f, BushTree::Type::Bush,     1, 0.0f, 0);
    BushTree bt2(100.0f, BushTree::Type::PineTree, 3, 0.0f, 1);
    REQUIRE(bt1.lightStrings().size() == 1);
    REQUIRE(bt2.lightStrings().size() == 3);
}

TEST_CASE("BushTree: light Y is within vertical bounds of object", "[bushtree]") {
    BushTree bush(100.0f, BushTree::Type::Bush,     1, 0.0f, 0);
    BushTree pine(200.0f, BushTree::Type::PineTree, 1, 0.0f, 1);

    for (auto& ls : bush.lightStrings()) {
        REQUIRE(ls->position.y >= bush.position.y);
        REQUIRE(ls->position.y <= LANE_GROUND_Y);
    }
    for (auto& ls : pine.lightStrings()) {
        REQUIRE(ls->position.y >= pine.position.y);
        REQUIRE(ls->position.y <= LANE_GROUND_Y);
    }
}

TEST_CASE("BushTree: light X starts within object bounds", "[bushtree]") {
    float wx = 300.0f;
    BushTree bt(wx, BushTree::Type::Bush, 2, 0.0f, 0);
    for (auto& ls : bt.lightStrings()) {
        REQUIRE(ls->position.x >= wx);
        REQUIRE(ls->position.x < wx + bt.width);
    }
}

TEST_CASE("BushTree: objectIndex returns value passed at construction", "[bushtree]") {
    BushTree bt(100.0f, BushTree::Type::PineTree, 1, 0.0f, 42);
    REQUIRE(bt.objectIndex() == 42);
}

TEST_CASE("BushTree: pine tree taller than bush", "[bushtree]") {
    BushTree bush(0.0f, BushTree::Type::Bush,     1, 0.0f, 0);
    BushTree pine(0.0f, BushTree::Type::PineTree, 1, 0.0f, 1);
    REQUIRE(pine.height > bush.height);
}

TEST_CASE("BushTree: both types sit on ground (position.y + height == LANE_GROUND_Y)", "[bushtree]") {
    BushTree bush(0.0f, BushTree::Type::Bush,     1, 0.0f, 0);
    BushTree pine(0.0f, BushTree::Type::PineTree, 1, 0.0f, 1);
    REQUIRE(bush.position.y + bush.height == Approx(LANE_GROUND_Y));
    REQUIRE(pine.position.y + pine.height == Approx(LANE_GROUND_Y));
}

TEST_CASE("BushTree: onDark callback fires when strand bitten off", "[bushtree]") {
    BushTree bt(100.0f, BushTree::Type::Bush, 1, 0.0f, 0);
    REQUIRE_FALSE(bt.lightStrings().empty());

    int callCount = 0;
    bt.lightStrings()[0]->onDark = [&](int, bool) { callCount++; };

    auto& ls = bt.lightStrings()[0];
    int bites = ls->bitesRequired();
    for (int i = 0; i < bites; ++i) ls->bite();

    // Advance cascade to completion
    for (int i = 0; i < 100; ++i) ls->update(0.1f);
    REQUIRE(callCount == 1);
}
