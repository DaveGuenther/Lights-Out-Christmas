#pragma once
#include "Entity.h"
#include "LightString.h"
#include <vector>
#include <memory>

namespace LightsOut {

enum class HouseStyle {
    Simple,          // basic house
    Elaborate,       // rich neighborhood
    TownSquare       // large community display
};

class House : public Entity {
public:
    // roofStrands/windowStrands/porchStrands: light strings per tier (0 = no lights on that tier)
    House(float worldX, float worldWidth, HouseStyle style,
          int houseIndex, int roofStrands, int windowStrands, int porchStrands,
          float tangledProb);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;

    bool isFullyDark() const;
    int  houseIndex() const { return m_houseIndex; }

    // Porch light
    bool  porchLightOn() const { return m_porchLightOn; }
    Vec2  porchLightPos() const;  // world-space center of porch light

    std::vector<std::shared_ptr<LightString>>& lightStrings() { return m_lightStrings; }

    // Triggered when player gets near with a homeowner on alert
    void triggerPorchLight();
    void resetPorchLight(float delaySeconds);

    // Color palette for this house's decorations
    Color wallColor;
    Color roofColor;

private:
    int        m_houseIndex;
    HouseStyle m_style;
    bool       m_porchLightOn    = false;
    float      m_porchResetTimer = 0.0f;
    bool       m_porchResetting  = false;

    std::vector<std::shared_ptr<LightString>> m_lightStrings;

    void placeRoofLights(int count, float tangledProb);
    void placeWindowLights(int count, float tangledProb);
    void placePorchLights(int count, float tangledProb);
    void drawHouseBody(SDL_Renderer* renderer, float screenX) const;
    void drawPorchLight(SDL_Renderer* renderer, float screenX) const;
};

}  // namespace LightsOut
