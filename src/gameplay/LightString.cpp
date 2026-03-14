#include "gameplay/LightString.h"
#include "core/Constants.h"
#include "core/SpriteRegistry.h"
#include <cmath>
#include <numbers>
#include <algorithm>
#include <random>

namespace LightsOut {

static std::mt19937 s_rng(42);

static const Color k_lightPalette[] = {
    {220, 60,  60},   // 0: red
    {60,  200, 60},   // 1: green
    {60,  100, 220},  // 2: blue
    {255, 220, 50},   // 3: yellow
    {240, 240, 240},  // 4: white
    {200, 80,  200},  // 5: purple
    {255, 140, 40},   // 6: orange
};

static const char* k_bulbOnSprites[] = {
    "bulb_red_on", "bulb_green_on", "bulb_blue_on", "bulb_yellow_on",
    "bulb_white_on", "bulb_purple_on", "bulb_orange_on"
};

static int randomLightColorIdx() {
    std::uniform_int_distribution<int> dist(0, 6);
    return dist(s_rng);
}

LightString::LightString(float worldX, float worldY, float length,
                         bool tangled, int houseIndex, LaneType lane)
    : Entity(TAG_LIGHT)
    , m_tangled(tangled)
    , m_houseIndex(houseIndex)
    , m_lane(lane)
    , m_bitesRequired(tangled ? LIGHT_BITES_TANGLED : 1)
{
    position.x = worldX;
    position.y = worldY;
    width  = length;
    height = LIGHT_BULB_SIZE * 3.0f;

    int count = static_cast<int>(length / LIGHT_SPACING);
    if (count < 2) count = 2;
    m_bulbs.reserve(count);

    for (int i = 0; i < count; ++i) {
        LightBulb b;
        b.position = {worldX + static_cast<float>(i) * LIGHT_SPACING, worldY};
        b.colorIdx = randomLightColorIdx();
        b.color    = k_lightPalette[b.colorIdx];
        b.state    = LightState::On;
        m_bulbs.push_back(b);
    }
}

LightString::LightString(const std::vector<Vec2>& path, LaneType lane,
                         bool tangled, int houseIndex)
    : Entity(TAG_LIGHT)
    , m_tangled(tangled)
    , m_houseIndex(houseIndex)
    , m_lane(lane)
    , m_bitesRequired(tangled ? LIGHT_BITES_TANGLED : 1)
{
    if (path.size() < 2) return;

    // Bounding box for entity extents
    float minX = path[0].x, maxX = path[0].x;
    float minY = path[0].y, maxY = path[0].y;
    for (const auto& p : path) {
        minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
    }
    position.x = minX;
    position.y = minY;
    width  = std::max(maxX - minX, 1.0f);
    height = std::max(maxY - minY, LIGHT_BULB_SIZE * 3.0f);

    // Place bulbs using euclidean-distance sampling: walk the path and emit a
    // bulb whenever we have moved >= LIGHT_SPACING pixels from the last one.
    // This prevents arc-length inflation from path zigzags (which would produce
    // hundreds of tightly-packed bulbs and create a solid blob).
    auto placeBulb = [&](const Vec2& pos) {
        LightBulb b;
        b.position = pos;
        b.colorIdx = randomLightColorIdx();
        b.color    = k_lightPalette[b.colorIdx];
        b.state    = LightState::On;
        m_bulbs.push_back(b);
    };

    Vec2 lastPlaced = path[0];
    placeBulb(path[0]);

    for (size_t i = 1; i < path.size(); ++i) {
        float dx   = path[i].x - lastPlaced.x;
        float dy   = path[i].y - lastPlaced.y;
        float dist = std::sqrt(dx*dx + dy*dy);
        if (dist >= LIGHT_SPACING) {
            placeBulb(path[i]);
            lastPlaced = path[i];
        }
    }

    // Always end the strand with a bulb at the last path point
    const Vec2& endPt = path.back();
    float ex = endPt.x - lastPlaced.x, ey = endPt.y - lastPlaced.y;
    if (std::sqrt(ex*ex + ey*ey) >= LIGHT_BULB_SIZE)
        placeBulb(endPt);
}

void LightString::update(float dt) {
    if (m_cascading) updateCascade(dt);

    // Update flickering bulbs
    for (auto& b : m_bulbs) {
        if (b.state == LightState::Flickering) {
            b.flickerTimer -= dt;
            if (b.flickerTimer <= 0.0f) b.state = LightState::Off;
        }
    }

    // Update spark particles
    updateSparks(dt);
}

void LightString::updateSparks(float dt) {
    for (auto& s : m_sparks) {
        s.pos.x += s.vel.x * dt;
        s.pos.y += s.vel.y * dt;
        s.vel.y += 40.0f * dt;  // gravity
        s.life  -= dt;
    }
    m_sparks.erase(
        std::remove_if(m_sparks.begin(), m_sparks.end(),
                       [](const Spark& s) { return s.life <= 0.0f; }),
        m_sparks.end());
}

void LightString::render(SDL_Renderer* renderer, float cameraX) {
    if (m_bulbs.empty()) return;
    if (isFullyOff()) return;  // don't draw wire or knot for dark strands

    // Draw wire between bulbs — straight line using both endpoints' actual y
    if (m_bulbs.size() > 1) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 180);
        for (size_t i = 0; i + 1 < m_bulbs.size(); ++i) {
            float x1 = m_bulbs[i].position.x   - cameraX;
            float y1 = m_bulbs[i].position.y;
            float x2 = m_bulbs[i+1].position.x - cameraX;
            float y2 = m_bulbs[i+1].position.y;
            SDL_RenderDrawLineF(renderer, x1, y1, x2, y2);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    // Draw bulbs: multi-layer illumination orb (soft glow) + bright core
    std::uniform_real_distribution<float> flicker(0.0f, 1.0f);
    for (const auto& b : m_bulbs) {
        if (b.state == LightState::Off) continue;
        if (b.state == LightState::Flickering && flicker(s_rng) > 0.5f) continue;

        float sx = b.position.x - cameraX;
        float sy = b.position.y;

        uint8_t r = b.color.r, g = b.color.g, bl = b.color.b;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // Outer soft glow — 9×9
        SDL_SetRenderDrawColor(renderer, r, g, bl, 18);
        SDL_FRect glow4 = {sx - 4.5f, sy - 4.5f, 9.0f, 9.0f};
        SDL_RenderFillRectF(renderer, &glow4);

        // Mid-outer glow — 7×7
        SDL_SetRenderDrawColor(renderer, r, g, bl, 35);
        SDL_FRect glow3 = {sx - 3.5f, sy - 3.5f, 7.0f, 7.0f};
        SDL_RenderFillRectF(renderer, &glow3);

        // Mid glow — 5×5
        SDL_SetRenderDrawColor(renderer, r, g, bl, 70);
        SDL_FRect glow2 = {sx - 2.5f, sy - 2.5f, 5.0f, 5.0f};
        SDL_RenderFillRectF(renderer, &glow2);

        // Inner halo — 3×3
        SDL_SetRenderDrawColor(renderer, r, g, bl, 140);
        SDL_FRect glow1 = {sx - 1.5f, sy - 1.5f, 3.0f, 3.0f};
        SDL_RenderFillRectF(renderer, &glow1);

        // Bright white-tinted core — 2×2
        SDL_SetRenderDrawColor(renderer,
            static_cast<uint8_t>(std::min(255, static_cast<int>(r) + 80)),
            static_cast<uint8_t>(std::min(255, static_cast<int>(g) + 80)),
            static_cast<uint8_t>(std::min(255, static_cast<int>(bl) + 80)),
            255);
        SDL_FRect core = {sx - 1.0f, sy - 1.0f, 2.0f, 2.0f};
        SDL_RenderFillRectF(renderer, &core);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // Bulb sprite drawn on top of glow
        SpriteRegistry::draw(renderer, k_bulbOnSprites[b.colorIdx], sx - 2.0f, sy - 3.0f);
    }

    // Tangled indicator — wire_knot sprite
    if (m_tangled && m_bitesLanded < m_bitesRequired) {
        float cx = position.x + width * 0.5f - cameraX;
        float cy = position.y - 3.0f;
        SpriteRegistry::draw(renderer, "wire_knot", cx - 3.0f, cy - 3.0f);
    }

    // Render sparks
    for (const auto& s : m_sparks) {
        float alpha = std::max(0.0f, s.life / SPARK_DURATION) * 255.0f;
        SDL_SetRenderDrawColor(renderer, s.color.r, s.color.g, s.color.b,
                               static_cast<uint8_t>(alpha));
        SDL_RenderDrawPointF(renderer, s.pos.x - cameraX, s.pos.y);
    }
}

bool LightString::bite() {
    if (isFullyOff()) return false;
    if (m_cascading) return false;

    m_bitesLanded++;
    if (m_bitesLanded >= m_bitesRequired) {
        startCascade();
    }
    return true;
}

bool LightString::isFullyOff() const {
    for (const auto& b : m_bulbs)
        if (b.state != LightState::Off) return false;
    return true;
}

void LightString::startCascade() {
    m_cascading    = true;
    m_cascadeTimer = 0.0f;
    m_cascadeIndex = 0;

    // Spawn initial sparks at the bite point
    if (!m_bulbs.empty()) {
        size_t mid = m_bulbs.size() / 2;
        spawnSparks(m_bulbs[mid].position);
    }
}

void LightString::updateCascade(float dt) {
    m_cascadeTimer += dt;
    static constexpr float STEP = 0.03f;

    while (m_cascadeTimer >= STEP && m_cascadeIndex < static_cast<int>(m_bulbs.size())) {
        m_cascadeTimer -= STEP;
        auto& b = m_bulbs[m_cascadeIndex];
        b.state = LightState::Flickering;
        b.flickerTimer = 0.15f;
        spawnSparks(b.position);
        m_cascadeIndex++;
    }

    if (m_cascadeIndex >= static_cast<int>(m_bulbs.size())) {
        // Force all remaining bulbs off
        for (auto& b : m_bulbs) b.state = LightState::Off;
        m_cascading = false;

        if (onDark) {
            onDark(static_cast<int>(m_bulbs.size()), false);
        }
    }
}

void LightString::spawnSparks(const Vec2& at) {
    std::uniform_real_distribution<float> angleDist(0.0f,
        2.0f * std::numbers::pi_v<float>);
    std::uniform_real_distribution<float> speedDist(SPARK_SPEED * 0.5f, SPARK_SPEED);

    for (int i = 0; i < SPARK_PARTICLE_COUNT; ++i) {
        float angle = angleDist(s_rng);
        float speed = speedDist(s_rng);
        Spark s;
        s.pos   = at;
        s.vel   = {std::cos(angle) * speed, std::sin(angle) * speed - 20.0f};
        s.life  = SPARK_DURATION;
        // Spark color: mix between orange and the nearest bulb color
        if (!m_bulbs.empty()) {
            s.color = m_bulbs[std::min(m_cascadeIndex, (int)m_bulbs.size()-1)].color;
        } else {
            s.color = {255, 180, 50};
        }
        m_sparks.push_back(s);
    }
}

}  // namespace LightsOut
