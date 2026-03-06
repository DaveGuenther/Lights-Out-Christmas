#pragma once
#include "Entity.h"
#include "core/Types.h"
#include <vector>
#include <functional>

namespace LightsOut {

enum class LightState { On, Off, Flickering, Sparking };

struct LightBulb {
    Vec2       position;   // world-space
    Color      color;
    int        colorIdx = 0;  // 0-6 index into the 7-color palette (used for sprite lookup)
    LightState state = LightState::On;
    float      flickerTimer = 0.0f;
};

class LightString : public Entity {
public:
    LightString(float worldX, float worldY, float length,
                bool tangled = false, int houseIndex = -1);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;

    // Returns true if the bite succeeded (or progressed for tangled strands).
    // Triggers the cascade and calls onDark when the strand is fully off.
    bool bite();

    bool isFullyOff() const;
    bool isTangled() const { return m_tangled; }
    int  houseIndex() const { return m_houseIndex; }
    int  lightCount() const { return static_cast<int>(m_bulbs.size()); }
    int  bitesRequired() const { return m_bitesRequired; }
    int  bitesLanded() const { return m_bitesLanded; }

    // Called by GameWorld after all lights in a house go dark
    std::function<void(int houseIndex)> onHouseBlackout;
    // Called when this specific strand goes dark (passes # bulbs killed)
    std::function<void(int bulbCount, bool wasChain)> onDark;

private:
    void startCascade();
    void updateCascade(float dt);
    void updateSparks(float dt);

    std::vector<LightBulb> m_bulbs;
    bool  m_tangled;
    int   m_houseIndex;
    int   m_bitesRequired;
    int   m_bitesLanded = 0;

    // Cascade state
    bool  m_cascading = false;
    float m_cascadeTimer = 0.0f;
    int   m_cascadeIndex = 0;  // which bulb to turn off next

    // Spark FX
    struct Spark {
        Vec2  pos;
        Vec2  vel;
        float life;     // seconds remaining
        Color color;
    };
    std::vector<Spark> m_sparks;
    void spawnSparks(const Vec2& at);
};

}  // namespace LightsOut
