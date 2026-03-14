#pragma once
#include "Entity.h"
#include "LightString.h"
#include "BushTree.h"
#include "core/HouseAssetLoader.h"
#include "core/PlatformData.h"
#include <map>
#include <utility>
#include <vector>
#include <memory>

namespace LightsOut {

enum class HouseStyle {
    Simple,
    Elaborate,
    TownSquare
};

// Parsed data from the mask image: world-space point lists for each tier
struct HouseMaskData {
    std::vector<std::vector<Vec2>> bluePaths;    // top tier (Rooftop lane)
    std::vector<std::vector<Vec2>> yellowPaths;  // middle tier (Fence lane)
    std::vector<std::vector<Vec2>> redPaths;     // ground tier path lights
    std::vector<Vec2>              greenPoints;  // ground tier bush areas
};

class House : public Entity {
public:
    // Sprite-based constructor (new system)
    House(float worldX, HouseStyle style, int houseIndex,
          const HouseAsset& asset,
          bool hasTopTier, bool hasMiddleTier, bool hasGroundTier,
          int strands, float tangledProb, SDL_Renderer* renderer = nullptr,
          const std::vector<BushAsset>* bushAssets = nullptr);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;

    bool isFullyDark() const;
    int  houseIndex() const { return m_houseIndex; }

    bool  porchLightOn() const { return m_porchLightOn; }
    Vec2  porchLightPos() const;

    std::vector<std::shared_ptr<LightString>>& lightStrings() { return m_lightStrings; }
    std::vector<std::shared_ptr<BushTree>>&    bushTrees()    { return m_bushTrees; }
    const std::vector<Platform>&               platforms()    const { return m_platforms; }

    void triggerPorchLight();
    void resetPorchLight(float delaySeconds);

    float spriteWidth() const { return m_spriteWidth; }

    Color wallColor;
    Color roofColor;

private:
    int        m_houseIndex;
    HouseStyle m_style;
    bool       m_porchLightOn    = false;
    float      m_porchResetTimer = 0.0f;
    bool       m_porchResetting  = false;
    float      m_spriteWidth     = 256.0f;
    std::string m_spriteName;  // key registered with SpriteRegistry

    std::vector<std::shared_ptr<LightString>> m_lightStrings;
    std::vector<std::shared_ptr<BushTree>>    m_bushTrees;
    std::vector<Platform>                     m_platforms;

    // Parse the mask image and return paths/areas in world space
    HouseMaskData parseMask(const std::string& maskPath,
                            float worldX, float worldWidth,
                            SDL_Renderer* renderer = nullptr) const;

    // Extract one path per BFS-connected component from a pixel-space grid
    using IPos = std::pair<int,int>;
    std::vector<std::vector<Vec2>> extractPaths(
        const std::map<IPos, Vec2>& grid) const;

    void placeTierLights(const std::vector<std::vector<Vec2>>& paths,
                         LaneType lane, int strands, float tangledProb);
    void placeGroundLights(const HouseMaskData& mask, int strands, float tangledProb);
    void placeBushes(const std::vector<Vec2>& greenPoints, float worldX,
                     const std::vector<BushAsset>* bushAssets);

    void drawPorchLight(SDL_Renderer* renderer, float screenX) const;
};

}  // namespace LightsOut
