#include "gameplay/LightString.h"
#include "core/Constants.h"
#include <cmath>
#include <numbers>
#include <algorithm>
#include <random>

namespace LightsOut {

static std::mt19937 s_rng(42);

static Color randomLightColor() {
    static const Color palette[] = {
        {220, 60,  60},   // red
        {60,  200, 60},   // green
        {60,  100, 220},  // blue
        {255, 220, 50},   // yellow
        {240, 240, 240},  // white
        {200, 80,  200},  // purple
        {255, 140, 40},   // orange
    };
    std::uniform_int_distribution<int> dist(0, 6);
    return palette[dist(s_rng)];
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
        b.color    = randomLightColor();
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

    // Draw bulbs
    std::uniform_real_distribution<float> flicker(0.0f, 1.0f);
    for (const auto& b : m_bulbs) {
        if (b.state == LightState::Off) continue;
        if (b.state == LightState::Flickering && flicker(s_rng) > 0.5f) continue;

        float sx = b.position.x - cameraX;
        float sy = b.position.y;

        // Glow halo
        SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, 60);
        SDL_FRect glow = {sx - LIGHT_BULB_SIZE, sy - LIGHT_BULB_SIZE,
                          LIGHT_BULB_SIZE * 2.0f + 1.0f, LIGHT_BULB_SIZE * 2.0f + 1.0f};
        SDL_RenderFillRectF(renderer, &glow);

        // Bulb
        SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, 255);
        SDL_FRect bulb = {sx - LIGHT_BULB_SIZE * 0.5f, sy - LIGHT_BULB_SIZE * 0.5f,
                          LIGHT_BULB_SIZE, LIGHT_BULB_SIZE};
        SDL_RenderFillRectF(renderer, &bulb);
    }

    // Tangled indicator — draw a small knot mark if strands need multiple bites
    if (m_tangled && m_bitesLanded < m_bitesRequired) {
        SDL_SetRenderDrawColor(renderer, 180, 100, 40, 200);
        float cx = position.x + width * 0.5f - cameraX;
        float cy = position.y - 3.0f;
        SDL_FRect knot = {cx - 2.0f, cy - 2.0f, 4.0f, 4.0f};
        SDL_RenderFillRectF(renderer, &knot);
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
