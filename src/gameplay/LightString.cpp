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
                         bool tangled, int houseIndex)
    : Entity(TAG_LIGHT)
    , m_tangled(tangled)
    , m_houseIndex(houseIndex)
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

    // Draw wire between bulbs
    if (m_bulbs.size() > 1) {
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 200);
        for (size_t i = 0; i + 1 < m_bulbs.size(); ++i) {
            float x1 = m_bulbs[i].position.x - cameraX;
            float x2 = m_bulbs[i+1].position.x - cameraX;
            float y  = m_bulbs[i].position.y;
            // Draw slight droop
            float mid = (x1 + x2) * 0.5f;
            SDL_RenderDrawLineF(renderer, x1, y, mid, y + 2.0f);
            SDL_RenderDrawLineF(renderer, mid, y + 2.0f, x2, y);
        }
    }

    // Draw bulbs using sprites
    std::uniform_real_distribution<float> flicker(0.0f, 1.0f);
    for (const auto& b : m_bulbs) {
        if (b.state == LightState::Off) continue;
        if (b.state == LightState::Flickering && flicker(s_rng) > 0.5f) continue;

        float sx = b.position.x - cameraX;
        float sy = b.position.y;

        // Soft glow halo (procedural, color-matched)
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, 45);
        SDL_FRect glow = {sx - LIGHT_BULB_SIZE * 1.5f, sy - LIGHT_BULB_SIZE * 1.5f,
                          LIGHT_BULB_SIZE * 3.0f, LIGHT_BULB_SIZE * 3.0f};
        SDL_RenderFillRectF(renderer, &glow);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // Bulb sprite (4x6 pixels drawn centered at bulb position)
        const char* spriteName = (b.state == LightState::Flickering)
                                 ? "bulb_flicker"
                                 : k_bulbOnSprites[b.colorIdx];
        SpriteRegistry::draw(renderer, spriteName,
                             sx - 2.0f, sy - 3.0f);  // center 4x6 sprite on bulb pos
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
