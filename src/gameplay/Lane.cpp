#include "gameplay/Lane.h"
#include "core/Constants.h"
#include <cmath>

namespace LightsOut {

float laneY(LaneType lane) {
    switch (lane) {
    case LaneType::Rooftop: return LANE_ROOFTOP_Y;
    case LaneType::Fence:   return LANE_FENCE_Y;
    case LaneType::Ground:  return LANE_GROUND_Y;
    }
    return LANE_GROUND_Y;
}

LaneInfo laneInfo(LaneType lane) {
    LaneInfo info{};
    info.type = lane;
    info.y    = laneY(lane);
    switch (lane) {
    case LaneType::Rooftop:
        info.speed         = 1.0f;
        info.slippery      = true;
        info.hasCats       = true;
        info.hasOwl        = false;
        info.hasHomeowners = false;
        break;
    case LaneType::Fence:
        info.speed         = 1.0f;
        info.slippery      = false;
        info.hasCats       = true;
        info.hasOwl        = true;
        info.hasHomeowners = false;
        break;
    case LaneType::Ground:
        info.speed         = 1.0f;
        info.slippery      = false;
        info.hasCats       = false;
        info.hasOwl        = false;
        info.hasHomeowners = true;
        break;
    }
    return info;
}

bool isOnLane(LaneType lane, float playerCenterY, float tolerance) {
    return std::abs(playerCenterY - laneY(lane)) <= tolerance;
}

LaneType laneAbove(LaneType lane) {
    switch (lane) {
    case LaneType::Ground:  return LaneType::Fence;
    case LaneType::Fence:   return LaneType::Rooftop;
    case LaneType::Rooftop: return LaneType::Rooftop;
    }
    return LaneType::Rooftop;
}

LaneType laneBelow(LaneType lane) {
    switch (lane) {
    case LaneType::Rooftop: return LaneType::Fence;
    case LaneType::Fence:   return LaneType::Ground;
    case LaneType::Ground:  return LaneType::Ground;
    }
    return LaneType::Ground;
}

int laneIndex(LaneType lane) {
    return static_cast<int>(lane);
}

LaneType laneFromIndex(int index) {
    switch (index) {
    case 0: return LaneType::Rooftop;
    case 1: return LaneType::Fence;
    case 2: return LaneType::Ground;
    default: return LaneType::Ground;
    }
}

}  // namespace LightsOut
