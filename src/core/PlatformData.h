#pragma once
#include "gameplay/Lane.h"
#include <vector>
#include <string>

namespace LightsOut {

// A single one-way platform segment in world space.
// Only the squirrel's bottom edge collides with it (from above while falling).
struct Platform {
    float    x1;    // world-space left edge
    float    x2;    // world-space right edge
    float    y;     // world-space Y of the top surface (feet rest here)
    LaneType tier;  // Ground / Fence / Rooftop — determines currentLane when landed
};

// Parse a collision-map PNG and return all platform segments it contains.
//   entityX        : world-space X of the entity's left edge
//   entityTopY     : world-space Y of the entity's top edge
//   renderedWidth  : width the sprite is drawn at in world space (px)
//   renderedHeight : height the sprite is drawn at in world space (px)
//                    Pass 0 for either to use the image's own pixel dimensions (1:1).
std::vector<Platform> parsePlatforms(const std::string& collisionPath,
                                     float entityX, float entityTopY,
                                     float renderedWidth  = 0.0f,
                                     float renderedHeight = 0.0f);

}  // namespace LightsOut
