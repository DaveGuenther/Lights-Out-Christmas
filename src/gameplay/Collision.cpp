#include "gameplay/Collision.h"
#include <cmath>
#include <algorithm>

namespace LightsOut {
namespace Collision {

bool overlaps(const Rect& a, const Rect& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x &&
           a.y < b.y + b.h && a.y + a.h > b.y;
}

bool circlesOverlap(const Vec2& cA, float rA, const Vec2& cB, float rB) {
    float dx   = cA.x - cB.x;
    float dy   = cA.y - cB.y;
    float rSum = rA + rB;
    return (dx * dx + dy * dy) <= (rSum * rSum);
}

bool pointInCircle(const Vec2& point, const Vec2& center, float radius) {
    float dx = point.x - center.x;
    float dy = point.y - center.y;
    return (dx * dx + dy * dy) <= (radius * radius);
}

bool rectOverlapsCircle(const Rect& rect, const Vec2& center, float radius) {
    float nearX = std::max(rect.x, std::min(center.x, rect.x + rect.w));
    float nearY = std::max(rect.y, std::min(center.y, rect.y + rect.h));
    float dx = nearX - center.x;
    float dy = nearY - center.y;
    return (dx * dx + dy * dy) <= (radius * radius);
}

bool resolveAABB(const Rect& a, const Rect& b, Contact& contact) {
    if (!overlaps(a, b)) return false;

    float overlapX = std::min(a.x + a.w, b.x + b.w) - std::max(a.x, b.x);
    float overlapY = std::min(a.y + a.h, b.y + b.h) - std::max(a.y, b.y);

    if (overlapX < overlapY) {
        contact.depth  = overlapX;
        contact.normal = (a.x < b.x) ? Vec2{-1.0f, 0.0f} : Vec2{1.0f, 0.0f};
    } else {
        contact.depth  = overlapY;
        contact.normal = (a.y < b.y) ? Vec2{0.0f, -1.0f} : Vec2{0.0f, 1.0f};
    }
    return true;
}

Rect expand(const Rect& r, float delta) {
    return {r.x - delta, r.y - delta, r.w + delta * 2.0f, r.h + delta * 2.0f};
}

}  // namespace Collision
}  // namespace LightsOut
