#include "rendering/Particles.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <random>

namespace LightsOut {

static std::mt19937 s_particleRng(0xC0FFEE);

void ParticleSystem::update(float dt) {
    for (auto& p : m_particles) {
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.velocity.y += 60.0f * dt;  // gravity
        p.life -= dt;
    }
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
                       [](const Particle& p) { return p.life <= 0.0f; }),
        m_particles.end());

    for (auto& tp : m_textParticles) {
        tp.worldPos.y -= 20.0f * dt;  // float upward
        tp.life -= dt;
    }
    m_textParticles.erase(
        std::remove_if(m_textParticles.begin(), m_textParticles.end(),
                       [](const TextParticle& tp) { return tp.life <= 0.0f; }),
        m_textParticles.end());
}

void ParticleSystem::render(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (const auto& p : m_particles) {
        float alpha = std::max(0.0f, p.life / p.maxLife) * 255.0f;
        SDL_SetRenderDrawColor(renderer, p.color.r, p.color.g, p.color.b,
                               static_cast<uint8_t>(alpha));
        float s = p.size;
        SDL_FRect rect = {p.position.x - s * 0.5f - m_cameraX,
                          p.position.y - s * 0.5f, s, s};
        SDL_RenderFillRectF(renderer, &rect);
    }

    // Text particles (score popups) — drawn as simple colored pixels
    for (const auto& tp : m_textParticles) {
        float alpha = std::max(0.0f, tp.life / tp.maxLife);
        uint8_t a = static_cast<uint8_t>(alpha * 220.0f);
        SDL_SetRenderDrawColor(renderer, tp.color.r, tp.color.g, tp.color.b, a);
        // Draw "+" indicator as a cross
        float sx = tp.worldPos.x - m_cameraX;
        float sy = tp.worldPos.y;
        SDL_RenderDrawPointF(renderer, sx,     sy);
        SDL_RenderDrawPointF(renderer, sx + 1, sy);
        SDL_RenderDrawPointF(renderer, sx - 1, sy);
        SDL_RenderDrawPointF(renderer, sx,     sy + 1);
        SDL_RenderDrawPointF(renderer, sx,     sy - 1);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void ParticleSystem::emitSparks(Vec2 worldPos, Color color, int count) {
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> speedDist(15.0f, 45.0f);
    std::uniform_real_distribution<float> sizeDist(0.8f, 2.0f);

    for (int i = 0; i < count; ++i) {
        float angle = angleDist(s_particleRng);
        float speed = speedDist(s_particleRng);
        Particle p;
        p.position = worldPos;
        p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed - 15.0f};
        p.color    = color;
        p.life     = 0.35f + speedDist(s_particleRng) * 0.005f;
        p.maxLife  = p.life;
        p.size     = sizeDist(s_particleRng);
        m_particles.push_back(p);
    }
}

void ParticleSystem::emitScorePopup(Vec2 worldPos, int score, float /*cameraX*/) {
    TextParticle tp;
    tp.worldPos = worldPos;
    tp.text     = "+" + std::to_string(score);
    tp.life     = 1.2f;
    tp.maxLife  = tp.life;
    // Color based on score magnitude
    if (score >= 500) tp.color = {255, 200, 50};
    else if (score >= 100) tp.color = {100, 255, 100};
    else tp.color = {200, 200, 255};
    m_textParticles.push_back(tp);
}

void ParticleSystem::clear() {
    m_particles.clear();
    m_textParticles.clear();
}

}  // namespace LightsOut
