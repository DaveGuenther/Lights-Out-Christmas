#pragma once
#include "core/Types.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>

namespace LightsOut {

struct Particle {
    Vec2  position;
    Vec2  velocity;
    Color color;
    float life;       // seconds remaining
    float maxLife;    // for alpha fade
    float size;       // radius in render pixels
};

class ParticleSystem {
public:
    ParticleSystem() = default;

    void update(float dt);
    void render(SDL_Renderer* renderer) const;

    // Emit a burst of spark particles at a world position
    void emitSparks(Vec2 worldPos, Color color, int count = 8);

    // Emit a score-popup text particle (rendered as pixel text)
    void emitScorePopup(Vec2 worldPos, int score, float cameraX);

    // Camera offset applied at render time
    void setCameraX(float x) { m_cameraX = x; }

    void clear();
    int  count() const { return static_cast<int>(m_particles.size()); }

private:
    std::vector<Particle> m_particles;
    float m_cameraX = 0.0f;

    struct TextParticle {
        Vec2        worldPos;
        std::string text;
        Color       color;
        float       life;
        float       maxLife;
    };
    std::vector<TextParticle> m_textParticles;
};

}  // namespace LightsOut
