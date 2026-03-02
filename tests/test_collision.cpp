#include <catch2/catch_all.hpp>
#include "gameplay/Collision.h"

using namespace LightsOut;
using namespace LightsOut::Collision;

TEST_CASE("Collision: overlapping rects", "[collision]") {
    Rect a{0, 0, 10, 10};
    Rect b{5, 5, 10, 10};
    REQUIRE(overlaps(a, b));
    REQUIRE(overlaps(b, a));
}

TEST_CASE("Collision: non-overlapping rects", "[collision]") {
    Rect a{0, 0, 10, 10};
    Rect b{20, 20, 10, 10};
    REQUIRE_FALSE(overlaps(a, b));
}

TEST_CASE("Collision: touching edge is not overlapping", "[collision]") {
    Rect a{0, 0, 10, 10};
    Rect b{10, 0, 10, 10};  // touching right edge
    REQUIRE_FALSE(overlaps(a, b));
}

TEST_CASE("Collision: contained rect overlaps", "[collision]") {
    Rect a{0, 0, 20, 20};
    Rect b{5, 5, 5, 5};  // inside a
    REQUIRE(overlaps(a, b));
}

TEST_CASE("Collision: circles overlap", "[collision]") {
    Vec2 c1{0, 0}, c2{5, 0};
    REQUIRE(circlesOverlap(c1, 3.0f, c2, 3.0f));  // overlap by 1
}

TEST_CASE("Collision: circles don't overlap", "[collision]") {
    Vec2 c1{0, 0}, c2{10, 0};
    REQUIRE_FALSE(circlesOverlap(c1, 3.0f, c2, 3.0f));
}

TEST_CASE("Collision: circles just touching", "[collision]") {
    Vec2 c1{0, 0}, c2{6, 0};
    // distance = 6, sum radii = 6 → exactly on boundary → overlaps (<=)
    REQUIRE(circlesOverlap(c1, 3.0f, c2, 3.0f));
}

TEST_CASE("Collision: point in circle", "[collision]") {
    Vec2 p{1, 1}, c{0, 0};
    REQUIRE(pointInCircle(p, c, 2.0f));
    REQUIRE_FALSE(pointInCircle(p, c, 1.0f));
}

TEST_CASE("Collision: rect overlaps circle", "[collision]") {
    Rect r{0, 0, 10, 10};
    Vec2 c{5, 5};
    REQUIRE(rectOverlapsCircle(r, c, 1.0f));  // center inside rect
    REQUIRE(rectOverlapsCircle(r, {15, 5}, 6.0f));  // close to right edge
    REQUIRE_FALSE(rectOverlapsCircle(r, {20, 5}, 5.0f));  // too far
}

TEST_CASE("Collision: expand rect", "[collision]") {
    Rect r{10, 10, 20, 20};
    Rect e = expand(r, 5.0f);
    REQUIRE(e.x == Approx(5.0f));
    REQUIRE(e.y == Approx(5.0f));
    REQUIRE(e.w == Approx(30.0f));
    REQUIRE(e.h == Approx(30.0f));
}

TEST_CASE("Collision: resolve AABB returns normal", "[collision]") {
    Rect a{0, 0, 10, 10};
    Rect b{8, 0, 10, 10};  // 2px overlap in X
    Contact c;
    REQUIRE(resolveAABB(a, b, c));
    REQUIRE(c.depth == Approx(2.0f));
    // Normal should be in X direction
    REQUIRE(c.normal.x != Approx(0.0f));
    REQUIRE(c.normal.y == Approx(0.0f));
}

TEST_CASE("Collision: resolve AABB no overlap", "[collision]") {
    Rect a{0, 0, 10, 10};
    Rect b{20, 0, 10, 10};
    Contact c;
    REQUIRE_FALSE(resolveAABB(a, b, c));
}

TEST_CASE("Vec2: basic operations", "[types]") {
    Vec2 a{3, 4};
    REQUIRE(a.length() == Approx(5.0f));

    Vec2 b = a.normalized();
    REQUIRE(b.length() == Approx(1.0f));

    Vec2 sum = a + Vec2{1, 1};
    REQUIRE(sum.x == Approx(4.0f));
    REQUIRE(sum.y == Approx(5.0f));
}

TEST_CASE("Rect: contains point", "[types]") {
    Rect r{0, 0, 10, 10};
    REQUIRE(r.contains({5, 5}));
    REQUIRE_FALSE(r.contains({15, 5}));
}

TEST_CASE("Rect: center", "[types]") {
    Rect r{0, 0, 10, 10};
    Vec2 c = r.center();
    REQUIRE(c.x == Approx(5.0f));
    REQUIRE(c.y == Approx(5.0f));
}
