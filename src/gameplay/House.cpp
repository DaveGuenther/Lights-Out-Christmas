#include "gameplay/House.h"
#include "core/Constants.h"
#include "core/SpriteRegistry.h"
#include "core/PlatformData.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <random>
#include <set>

namespace LightsOut {

static std::mt19937 s_houseRng(123);

// ─── Color matching helpers ───────────────────────────────────────────────────
static bool isBlue(uint8_t r, uint8_t g, uint8_t b) {
    return b >= 150 && b > r + 60 && b > g + 40;
}
static bool isYellow(uint8_t r, uint8_t g, uint8_t b) {
    return r > 150 && g > 150 && b < 100;
}
static bool isRed(uint8_t r, uint8_t g, uint8_t b) {
    return r > 150 && g < 100 && b < 100;
}
static bool isGreen(uint8_t r, uint8_t g, uint8_t b) {
    return g > 150 && r < 100 && b < 100;
}

// ─── Constructor ─────────────────────────────────────────────────────────────
House::House(float worldX, HouseStyle style, int houseIndex,
             const HouseAsset& asset,
             bool hasTopTier, bool hasMiddleTier, bool hasGroundTier,
             int strands, float tangledProb, SDL_Renderer* renderer,
             const std::vector<BushAsset>* bushAssets)
    : Entity(TAG_HOUSE)
    , m_houseIndex(houseIndex)
    , m_style(style)
    , m_spriteWidth(asset.pixelWidth)
    , m_spriteName(asset.name)
{
    position.x = worldX;
    position.y = HOUSE_GROUND_Y - HOUSE_HEIGHT;
    width  = asset.pixelWidth;
    height = HOUSE_HEIGHT;
    (void)m_style;  // reserved for future style-based rendering variants

    HouseMaskData mask = parseMask(asset.maskPath, worldX, asset.pixelWidth, renderer);

    // Always create lights for any tier that has mask data — ensures painted
    // sprite lights always have corresponding interactive LightString objects.
    if (!mask.bluePaths.empty())
        placeTierLights(mask.bluePaths, LaneType::Rooftop, strands, tangledProb);

    if (!mask.yellowPaths.empty())
        placeTierLights(mask.yellowPaths, LaneType::Fence, strands, tangledProb);

    placeGroundLights(mask, strands, tangledProb);

    (void)hasTopTier; (void)hasMiddleTier; (void)hasGroundTier;

    placeBushes(mask.greenPoints, worldX, bushAssets);

    // Parse collision map → platforms (pass rendered size so pixel coords are scaled correctly)
    m_platforms = parsePlatforms(asset.collisionPath, worldX, position.y,
                                 m_spriteWidth, HOUSE_HEIGHT);
}

// ─── Update ──────────────────────────────────────────────────────────────────
void House::update(float dt) {
    for (auto& ls : m_lightStrings) ls->update(dt);
    for (auto& bt : m_bushTrees)    bt->update(dt);

    if (m_porchResetting) {
        m_porchResetTimer -= dt;
        if (m_porchResetTimer <= 0.0f) {
            m_porchLightOn   = false;
            m_porchResetting = false;
        }
    }
}

// ─── Render ──────────────────────────────────────────────────────────────────
void House::render(SDL_Renderer* renderer, float cameraX) {
    float sx = position.x - cameraX;

    SpriteRegistry::draw(renderer, m_spriteName.c_str(),
                         sx, position.y,
                         m_spriteWidth, HOUSE_HEIGHT);

    drawPorchLight(renderer, sx);

    for (auto& ls : m_lightStrings) ls->render(renderer, cameraX);
    for (auto& bt : m_bushTrees)    bt->render(renderer, cameraX);
}

bool House::isFullyDark() const {
    for (const auto& ls : m_lightStrings)
        if (!ls->isFullyOff()) return false;
    return !m_lightStrings.empty();
}

Vec2 House::porchLightPos() const {
    return {position.x + m_spriteWidth * 0.15f, HOUSE_GROUND_Y - 15.0f};
}

void House::triggerPorchLight() {
    m_porchLightOn   = true;
    m_porchResetting = false;
}

void House::resetPorchLight(float delaySeconds) {
    m_porchResetTimer = delaySeconds;
    m_porchResetting  = true;
}

// ─── Mask parsing ─────────────────────────────────────────────────────────────
HouseMaskData House::parseMask(const std::string& maskPath,
                                float worldX, float worldWidth,
                                SDL_Renderer* /*renderer*/) const
{
    HouseMaskData result;

    SDL_Surface* surf = IMG_Load(maskPath.c_str());
    if (!surf) return result;

    SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surf);
    if (!rgba) return result;

    int imgW = rgba->w;
    int imgH = rgba->h;

    // Build pixel-space grids (image coords → world Vec2) for BFS path extraction
    std::map<IPos, Vec2> blueGrid, yellowGrid, redGrid;
    std::vector<Vec2> greenPixels;

    SDL_LockSurface(rgba);
    const uint8_t* pixels = static_cast<const uint8_t*>(rgba->pixels);
    int pitch = rgba->pitch;

    for (int y = 0; y < imgH; ++y) {
        for (int x = 0; x < imgW; ++x) {
            const uint8_t* px = pixels + y * pitch + x * 4;
            uint8_t r = px[0], g = px[1], b = px[2], a = px[3];
            if (a < 128) continue;

            float wx = worldX + (static_cast<float>(x) / static_cast<float>(imgW)) * worldWidth;
            float wy = (static_cast<float>(y) / static_cast<float>(imgH)) * HOUSE_HEIGHT
                       + (HOUSE_GROUND_Y - HOUSE_HEIGHT);
            Vec2 pt{wx, wy};

            if      (isBlue  (r, g, b)) blueGrid  [{x, y}] = pt;
            else if (isYellow(r, g, b)) yellowGrid [{x, y}] = pt;
            else if (isRed   (r, g, b)) redGrid    [{x, y}] = pt;
            else if (isGreen (r, g, b)) greenPixels.push_back(pt);
        }
    }
    SDL_UnlockSurface(rgba);
    SDL_FreeSurface(rgba);

    result.bluePaths   = extractPaths(blueGrid);
    result.yellowPaths = extractPaths(yellowGrid);
    result.redPaths    = extractPaths(redGrid);
    result.greenPoints = greenPixels;

    return result;
}

// ─── Path extraction ──────────────────────────────────────────────────────────
// One path per BFS-connected component (image pixel space), reduced to a
// centerline so thick strokes don't produce stacked duplicate bulbs.
// Scan order: bottom-to-top (descending Y), then left-to-right (ascending X).
std::vector<std::vector<Vec2>> House::extractPaths(
    const std::map<IPos, Vec2>& grid) const
{
    std::vector<std::vector<Vec2>> paths;
    if (grid.empty()) return paths;

    // Scan order: bottom-to-top, left-to-right
    std::vector<IPos> scanOrder;
    scanOrder.reserve(grid.size());
    for (const auto& [key, _] : grid) scanOrder.push_back(key);
    std::sort(scanOrder.begin(), scanOrder.end(),
              [](const IPos& a, const IPos& b) {
                  if (a.second != b.second) return a.second > b.second;
                  return a.first < b.first;
              });

    static constexpr int kDx[] = {-1,-1,-1, 0, 0, 1, 1, 1};
    static constexpr int kDy[] = {-1, 0, 1,-1, 1,-1, 0, 1};

    std::set<IPos> visited;

    for (const IPos& seed : scanOrder) {
        if (visited.count(seed)) continue;

        // BFS — collect all 8-connected pixels in this component
        std::vector<Vec2> component;
        std::queue<IPos> q;
        q.push(seed);
        visited.insert(seed);
        while (!q.empty()) {
            auto [cx, cy] = q.front(); q.pop();
            component.push_back(grid.at({cx, cy}));
            for (int d = 0; d < 8; ++d) {
                IPos nb{cx + kDx[d], cy + kDy[d]};
                if (!visited.count(nb) && grid.count(nb)) {
                    visited.insert(nb);
                    q.push(nb);
                }
            }
        }
        if (component.size() < 2) continue;

        // Reduce to centerline: sort by world X, take median world Y per column
        std::sort(component.begin(), component.end(),
                  [](const Vec2& a, const Vec2& b){ return a.x < b.x; });

        std::vector<Vec2> centerline;
        size_t i = 0;
        while (i < component.size()) {
            float colX = component[i].x;
            size_t j = i;
            while (j < component.size() && component[j].x == colX) ++j;
            std::vector<float> ys;
            ys.reserve(j - i);
            for (size_t k = i; k < j; ++k) ys.push_back(component[k].y);
            std::sort(ys.begin(), ys.end());
            centerline.push_back({colX, ys[ys.size() / 2]});
            i = j;
        }

        if (centerline.size() >= 2) paths.push_back(std::move(centerline));
    }
    return paths;
}

// ─── Light placement ──────────────────────────────────────────────────────────
void House::placeTierLights(const std::vector<std::vector<Vec2>>& paths,
                             LaneType lane, int /*strands*/, float tangledProb)
{
    std::uniform_real_distribution<float> tangledDist(0.0f, 1.0f);
    // Create a LightString for every connected path in the mask — no cap,
    // so every visually distinct run of lights becomes an interactive strand.
    for (const auto& path : paths) {
        bool tangled = tangledDist(s_houseRng) < tangledProb;
        auto ls = std::make_shared<LightString>(path, lane, tangled, m_houseIndex);
        m_lightStrings.push_back(ls);
    }
}

void House::placeGroundLights(const HouseMaskData& mask, int /*strands*/, float tangledProb)
{
    std::uniform_real_distribution<float> tangledDist(0.0f, 1.0f);
    for (const auto& path : mask.redPaths) {
        bool tangled = tangledDist(s_houseRng) < tangledProb;
        auto ls = std::make_shared<LightString>(path, LaneType::Ground, tangled, m_houseIndex);
        m_lightStrings.push_back(ls);
    }
}

// ─── Bush placement ───────────────────────────────────────────────────────────
void House::placeBushes(const std::vector<Vec2>& greenPoints,
                         float /*worldX*/,
                         const std::vector<BushAsset>* bushAssets)
{
    if (greenPoints.empty() || !bushAssets || bushAssets->empty()) return;

    // Cluster green pixels by proximity to find distinct bush slots
    float clusterRadius = 16.0f;
    std::vector<bool> used(greenPoints.size(), false);
    std::vector<float> clusterCenters;

    for (size_t i = 0; i < greenPoints.size(); ++i) {
        if (used[i]) continue;
        used[i] = true;

        float cx    = greenPoints[i].x;
        int   count = 1;

        for (size_t j = i + 1; j < greenPoints.size(); ++j) {
            if (!used[j]) {
                float dx = greenPoints[j].x - cx;
                float dy = greenPoints[j].y - greenPoints[i].y;
                if (std::sqrt(dx*dx + dy*dy) < clusterRadius) {
                    cx = (cx * static_cast<float>(count) + greenPoints[j].x)
                         / static_cast<float>(count + 1);
                    ++count;
                    used[j] = true;
                }
            }
        }
        clusterCenters.push_back(cx);
    }

    // Limit to 5 bushes per house
    if (clusterCenters.size() > 5) clusterCenters.resize(5);

    static int s_bushIdx = 0;
    std::uniform_int_distribution<size_t> assetDist(0, bushAssets->size() - 1);

    for (float cx : clusterCenters) {
        const BushAsset& asset = (*bushAssets)[assetDist(s_houseRng)];
        auto bt = std::make_shared<BushTree>(cx, asset, 0.0f, s_bushIdx++);
        m_bushTrees.push_back(bt);
    }
}

// ─── Porch light ─────────────────────────────────────────────────────────────
void House::drawPorchLight(SDL_Renderer* renderer, float screenX) const {
    if (!m_porchLightOn) return;

    float lpx = screenX + m_spriteWidth * 0.15f;
    float lpy = HOUSE_GROUND_Y - 30.0f;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < 5; ++i) {
        float fi = static_cast<float>(i);
        SDL_SetRenderDrawColor(renderer, 255, 230, 120,
                               static_cast<uint8_t>(50 - i * 8));
        SDL_FRect cone = {lpx - fi * 2.5f, lpy + fi * 4.0f,
                          fi * 5.0f + 3.0f, 4.0f};
        SDL_RenderFillRectF(renderer, &cone);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

}  // namespace LightsOut
