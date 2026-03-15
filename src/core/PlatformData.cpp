#include "core/PlatformData.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>

namespace LightsOut {

// Colour thresholds matching House.cpp mask parsing
static bool isRed   (uint8_t r, uint8_t g, uint8_t b) { return r > 150 && g < 100 && b < 100; }
static bool isYellow(uint8_t r, uint8_t g, uint8_t b) { return r > 150 && g > 150 && b < 100; }
static bool isBlue  (uint8_t r, uint8_t g, uint8_t b) { return b >= 150 && b > r + 60 && b > g + 40; }

std::vector<Platform> parsePlatforms(const std::string& collisionPath,
                                     float entityX, float entityTopY,
                                     float renderedWidth, float renderedHeight)
{
    std::vector<Platform> result;
    if (collisionPath.empty()) return result;

    SDL_Surface* surf = IMG_Load(collisionPath.c_str());
    if (!surf) return result;

    SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surf);
    if (!rgba) return result;

    const int imgW  = rgba->w;
    const int imgH  = rgba->h;
    const int pitch = rgba->pitch;

    // Scale pixel coords to match how the sprite is actually rendered.
    // If 0 is passed fall back to 1:1 (trees already use natural image dimensions).
    const float scaleX = (renderedWidth  > 0.0f) ? renderedWidth  / static_cast<float>(imgW) : 1.0f;
    const float scaleY = (renderedHeight > 0.0f) ? renderedHeight / static_cast<float>(imgH) : 1.0f;

    SDL_LockSurface(rgba);
    const uint8_t* pixels = static_cast<const uint8_t*>(rgba->pixels);

    // Column-scan approach: for each X column find ALL distinct colored stripes
    // per tier (a stripe = consecutive pixel rows of the same color in one column).
    // Record the topmost row of each stripe so that collision maps with multiple
    // separate lines of the same color (e.g. two roof peaks, multiple tree tiers)
    // each produce their own platforms.
    //
    // After collection, group (tier, stripeTopY) hits by Y-value then find
    // consecutive column runs to emit one Platform segment per run.

    static const LaneType kTierLane[] = {LaneType::Ground, LaneType::Fence, LaneType::Rooftop};

    // For each tier: map from image-row Y → sorted list of columns that have a
    // stripe starting at that Y.
    std::vector<std::map<int, std::vector<int>>> tierYCols(3);

    for (int px = 0; px < imgW; ++px) {
        for (int ti = 0; ti < 3; ++ti) {
            int stripeTop = -1;
            for (int py = 0; py < imgH; ++py) {
                const uint8_t* pxl = pixels + py * pitch + px * 4;
                uint8_t r = pxl[0], g = pxl[1], b = pxl[2], a = pxl[3];
                bool match = (a >= 128) && (
                    (ti == 0 && isRed   (r, g, b)) ||
                    (ti == 1 && isYellow(r, g, b)) ||
                    (ti == 2 && isBlue  (r, g, b)));

                if (match && stripeTop < 0) {
                    stripeTop = py;   // start of a new stripe
                } else if (!match && stripeTop >= 0) {
                    tierYCols[ti][stripeTop].push_back(px);
                    stripeTop = -1;
                }
            }
            if (stripeTop >= 0)
                tierYCols[ti][stripeTop].push_back(px);  // flush last stripe
        }
    }

    SDL_UnlockSurface(rgba);
    SDL_FreeSurface(rgba);

    // Build platform segments: for each (tier, Y), group consecutive columns
    // into contiguous runs → one Platform per run.
    for (int ti = 0; ti < 3; ++ti) {
        for (auto& [py, cols] : tierYCols[ti]) {
            std::sort(cols.begin(), cols.end());

            int runStart = cols[0];
            int prev     = cols[0];
            for (size_t i = 1; i <= cols.size(); ++i) {
                int cur = (i < cols.size()) ? cols[i] : -1;
                bool gap = (i == cols.size()) || (cur != prev + 1);
                if (gap) {
                    Platform p;
                    p.x1   = entityX    + static_cast<float>(runStart) * scaleX;
                    p.x2   = entityX    + static_cast<float>(prev + 1) * scaleX;
                    p.y    = entityTopY + static_cast<float>(py)        * scaleY;
                    p.tier = kTierLane[ti];
                    result.push_back(p);
                    if (i < cols.size()) runStart = cur;
                }
                if (i < cols.size()) prev = cur;
            }
        }
    }

    return result;
}

}  // namespace LightsOut
