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

class TreeEntity : public Entity {
public:
    // worldX      : left edge of tree in world space
    // bottomY     : Y coordinate of the tree's bottom edge (272–352)
    // asset       : selected tree asset (sprite/mask/collision paths + dimensions)
    // houseIndex  : used to route onDark callbacks (use a separate "tree" index)
    // tangledProb : probability that any light strand is tangled
    TreeEntity(float worldX, float bottomY,
               const TreeAsset& asset,
               int treeIndex, float tangledProb);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;

    const std::vector<Platform>&               platforms()    const { return m_platforms; }
    std::vector<std::shared_ptr<LightString>>& lightStrings()       { return m_lightStrings; }

    int treeIndex() const { return m_treeIndex; }

private:
    std::string m_spriteName;
    float       m_spriteW = 0.0f;
    float       m_spriteH = 0.0f;

    std::vector<Platform>                     m_platforms;
    std::vector<std::shared_ptr<LightString>> m_lightStrings;

    int m_treeIndex;
};

}  // namespace LightsOut
