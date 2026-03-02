#pragma once
#include "core/Types.h"

namespace LightsOut {
namespace Collision {

// AABB overlap test
bool overlaps(const Rect& a, const Rect& b);

// Circle overlap test
bool circlesOverlap(const Vec2& centerA, float radiusA,
                    const Vec2& centerB, float radiusB);

// Point in circle
bool pointInCircle(const Vec2& point, const Vec2& center, float radius);

// AABB vs circle
bool rectOverlapsCircle(const Rect& rect, const Vec2& circleCenter, float radius);

// Returns penetration depth and normal for resolving AABB collisions.
// Returns true if overlapping.
struct Contact {
    Vec2  normal;     // push A out of B along this direction
    float depth;
};
bool resolveAABB(const Rect& a, const Rect& b, Contact& contact);

// Expand a rect by delta on all sides (used for porch-light radius checks)
Rect expand(const Rect& r, float delta);

}  // namespace Collision
}  // namespace LightsOut
