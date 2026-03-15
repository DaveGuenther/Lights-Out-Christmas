#pragma once
#include "Entity.h"
#include "LightString.h"
#include "Lane.h"
#include "core/HouseAssetLoader.h"
#include "core/PlatformData.h"
#include <vector>
#include <memory>

struct SDL_Renderer;

namespace LightsOut {

class BushTree : public Entity {
public:
    BushTree(float worldX, const BushAsset& asset, float tangledProb, int idx);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;

    std::vector<std::shared_ptr<LightString>>& lightStrings() { return m_lights; }
    const std::vector<Platform>&               platforms()    const { return m_platforms; }
    int objectIndex() const { return m_idx; }

private:
    std::string m_spriteName;
    float       m_spriteW = 0.0f;
    float       m_spriteH = 0.0f;
    int         m_idx;

    std::vector<Platform>                     m_platforms;
    std::vector<std::shared_ptr<LightString>> m_lights;
};

}  // namespace LightsOut
