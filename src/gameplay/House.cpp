#include "gameplay/House.h"
#include "core/Constants.h"
#include "core/SpriteRegistry.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <random>

namespace LightsOut {

static std::mt19937 s_houseRng(123);

// ─── Color matching helpers ───────────────────────────────────────────────────
static bool isBlue(uint8_t r, uint8_t g, uint8_t b) {
    return b > 150 && r < 100 && g < 100;
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
             int strands, float tangledProb, SDL_Renderer* renderer)
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

    if (hasTopTier && !mask.bluePaths.empty())
        placeTierLights(mask.bluePaths, LaneType::Rooftop, strands, tangledProb);

    if (hasMiddleTier && !mask.yellowPaths.empty())
        placeTierLights(mask.yellowPaths, LaneType::Fence, strands, tangledProb);

    if (hasGroundTier)
        placeGroundLights(mask, strands, tangledProb);

    placeBushes(mask.greenPoints, worldX, asset.pixelWidth);
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

    std::vector<Vec2> bluePixels, yellowPixels, redPixels, greenPixels;

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
            if (isBlue(r, g, b))
                bluePixels.push_back(pt);
            else if (isYellow(r, g, b))
                yellowPixels.push_back(pt);
            else if (isRed(r, g, b))
                redPixels.push_back(pt);
            else if (isGreen(r, g, b))
                greenPixels.push_back(pt);
        }
    }
    SDL_UnlockSurface(rgba);
    SDL_FreeSurface(rgba);

    result.bluePaths   = extractPaths(bluePixels,   worldX, worldWidth, static_cast<float>(imgW));
    result.yellowPaths = extractPaths(yellowPixels, worldX, worldWidth, static_cast<float>(imgW));
    result.redPaths    = extractPaths(redPixels,    worldX, worldWidth, static_cast<float>(imgW));
    result.greenPoints = greenPixels;

    return result;
}

// ─── Path extraction ──────────────────────────────────────────────────────────
std::vector<std::vector<Vec2>> House::extractPaths(
    const std::vector<Vec2>& pixels, float /*worldX*/,
    float worldWidth, float maskImgWidth) const
{
    std::vector<std::vector<Vec2>> paths;
    if (pixels.empty()) return paths;

    std::vector<Vec2> sorted = pixels;
    std::sort(sorted.begin(), sorted.end(),
              [](const Vec2& a, const Vec2& b){ return a.x < b.x; });

    // Split into separate paths when there is a large horizontal gap
    float gapThreshold = (worldWidth / maskImgWidth) * 4.0f;

    std::vector<Vec2> current;
    current.push_back(sorted[0]);

    for (size_t i = 1; i < sorted.size(); ++i) {
        float dx = sorted[i].x - sorted[i-1].x;
        float dy = sorted[i].y - sorted[i-1].y;
        float dist = std::sqrt(dx*dx + dy*dy);
        if (dist > gapThreshold) {
            if (current.size() >= 2) paths.push_back(current);
            current.clear();
        }
        current.push_back(sorted[i]);
    }
    if (current.size() >= 2) paths.push_back(current);

    return paths;
}

// ─── Light placement ──────────────────────────────────────────────────────────
void House::placeTierLights(const std::vector<std::vector<Vec2>>& paths,
                             LaneType lane, int strands, float tangledProb)
{
    std::uniform_real_distribution<float> tangledDist(0.0f, 1.0f);
    int placed = 0;
    for (const auto& path : paths) {
        if (placed >= strands) break;
        bool tangled = tangledDist(s_houseRng) < tangledProb;
        auto ls = std::make_shared<LightString>(path, lane, tangled, m_houseIndex);
        m_lightStrings.push_back(ls);
        ++placed;
    }
}

void House::placeGroundLights(const HouseMaskData& mask, int strands, float tangledProb)
{
    std::uniform_real_distribution<float> tangledDist(0.0f, 1.0f);
    int placed = 0;
    for (const auto& path : mask.redPaths) {
        if (placed >= strands) break;
        bool tangled = tangledDist(s_houseRng) < tangledProb;
        auto ls = std::make_shared<LightString>(path, LaneType::Ground, tangled, m_houseIndex);
        m_lightStrings.push_back(ls);
        ++placed;
    }
}

// ─── Bush placement ───────────────────────────────────────────────────────────
void House::placeBushes(const std::vector<Vec2>& greenPoints,
                         float /*worldX*/, float /*worldWidth*/)
{
    if (greenPoints.empty()) return;

    float clusterRadius = 16.0f;
    std::vector<bool> used(greenPoints.size(), false);

    static int s_bushIdx = 0;
    std::uniform_real_distribution<float> typeDist(0.0f, 1.0f);

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

        BushTree::Type type = (typeDist(s_houseRng) < 0.4f)
                              ? BushTree::Type::PineTree
                              : BushTree::Type::Bush;
        auto bt = std::make_shared<BushTree>(cx, type, 0, 0.0f, s_bushIdx++);
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
