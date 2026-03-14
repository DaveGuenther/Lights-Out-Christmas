#include "gameplay/BushTree.h"
#include "core/Constants.h"
#include "core/SpriteRegistry.h"
#include "core/PlatformData.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <map>
#include <queue>
#include <random>
#include <set>

namespace LightsOut {

static std::mt19937 s_bushRng(0xB055);

static bool isBlue  (uint8_t r, uint8_t g, uint8_t b) { return b >= 150 && b > r + 60 && b > g + 40; }
static bool isYellow(uint8_t r, uint8_t g, uint8_t b) { return r > 150 && g > 150 && b < 100; }
static bool isRed   (uint8_t r, uint8_t g, uint8_t b) { return r > 150 && g < 100 && b < 100; }

static std::vector<std::vector<Vec2>> extractPaths(const std::vector<Vec2>& pixels)
{
    std::vector<std::vector<Vec2>> paths;
    if (pixels.empty()) return paths;

    using IPos = std::pair<int,int>;

    std::map<IPos, Vec2> grid;
    for (const auto& p : pixels) {
        int ix = static_cast<int>(std::round(p.x));
        int iy = static_cast<int>(std::round(p.y));
        grid[{ix, iy}] = p;
    }

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

BushTree::BushTree(float worldX, const BushAsset& asset, float tangledProb, int idx)
    : Entity(TAG_HOUSE)
    , m_spriteName(asset.name)
    , m_spriteW(asset.pixelWidth)
    , m_spriteH(asset.pixelHeight)
    , m_idx(idx)
{
    position.x = worldX;
    position.y = LANE_GROUND_Y - asset.pixelHeight;
    width      = asset.pixelWidth;
    height     = asset.pixelHeight;

    // Parse collision map for platforms
    m_platforms = parsePlatforms(asset.collisionPath, worldX, position.y,
                                 asset.pixelWidth, asset.pixelHeight);

    // Parse mask for light strands
    if (!asset.maskPath.empty()) {
        SDL_Surface* surf = IMG_Load(asset.maskPath.c_str());
        if (surf) {
            SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
            SDL_FreeSurface(surf);
            if (rgba) {
                int imgW  = rgba->w;
                int imgH  = rgba->h;
                int pitch = rgba->pitch;
                const uint8_t* pixels = static_cast<const uint8_t*>(rgba->pixels);

                std::vector<Vec2> bluePx, yellowPx, redPx;

                SDL_LockSurface(rgba);
                for (int py = 0; py < imgH; ++py) {
                    for (int px = 0; px < imgW; ++px) {
                        const uint8_t* p = pixels + py * pitch + px * 4;
                        uint8_t r = p[0], g = p[1], b = p[2], a = p[3];
                        if (a < 128) continue;
                        Vec2 pt{ worldX + static_cast<float>(px),
                                 position.y + static_cast<float>(py) };
                        if      (isBlue  (r, g, b)) bluePx.push_back(pt);
                        else if (isYellow(r, g, b)) yellowPx.push_back(pt);
                        else if (isRed   (r, g, b)) redPx.push_back(pt);
                    }
                }
                SDL_UnlockSurface(rgba);
                SDL_FreeSurface(rgba);

                std::uniform_real_distribution<float> tangleDist(0.0f, 1.0f);

                auto makeLights = [&](const std::vector<Vec2>& pxList, LaneType lane) {
                    for (const auto& path : extractPaths(pxList)) {
                        bool tangled = tangleDist(s_bushRng) < tangledProb;
                        auto ls = std::make_shared<LightString>(path, lane, tangled, idx);
                        m_lights.push_back(ls);
                    }
                };

                makeLights(redPx,    LaneType::Ground);
                makeLights(yellowPx, LaneType::Fence);
                makeLights(bluePx,   LaneType::Rooftop);
            }
        }
    }
}

void BushTree::update(float dt) {
    for (auto& ls : m_lights) ls->update(dt);
}

void BushTree::render(SDL_Renderer* renderer, float cameraX) {
    float sx = position.x - cameraX;
    SpriteRegistry::draw(renderer, m_spriteName.c_str(),
                         sx, position.y, m_spriteW, m_spriteH);
    for (auto& ls : m_lights) ls->render(renderer, cameraX);
}

}  // namespace LightsOut
