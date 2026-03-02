#pragma once
#include "core/Constants.h"
#include "core/Types.h"

namespace LightsOut {

struct LaneInfo {
    LaneType type;
    float    y;           // center Y in render space
    float    speed;       // scroll-speed multiplier (some lanes move at different rates)
    bool     slippery;    // ice patches — random per frame control inversion
    bool     hasOwl;      // owls lurk on branches
    bool     hasCats;     // cats can roam this lane
    bool     hasHomeowners; // homeowners can follow on ground
};

// Returns the static Y position for a lane type
float laneY(LaneType lane);

// Returns full info struct for a lane type
LaneInfo laneInfo(LaneType lane);

// Checks whether a given Y position (center) is considered "on" a lane
bool isOnLane(LaneType lane, float playerCenterY, float tolerance = 4.0f);

// Returns the lane type above the given lane (wraps at top → stays at Rooftop)
LaneType laneAbove(LaneType lane);

// Returns the lane type below the given lane (wraps at bottom → stays at Ground)
LaneType laneBelow(LaneType lane);

// Lane index (0 = Rooftop, 4 = Ground)
int laneIndex(LaneType lane);

// Lane from index
LaneType laneFromIndex(int index);

}  // namespace LightsOut
