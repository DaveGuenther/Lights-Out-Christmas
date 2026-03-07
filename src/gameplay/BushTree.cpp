#include "gameplay/BushTree.h"
#include "core/Constants.h"
#include "core/SpriteRegistry.h"
#include <SDL2/SDL.h>
#include <random>

namespace LightsOut {

static std::mt19937 s_bushRng(0xB055);

BushTree::BushTree(float worldX, Type type, int lightCount, float tangledProb, int idx)
    : Entity(TAG_HOUSE)  // reuse house tag so pruning/rendering works the same way
    , m_type(type)
    , m_idx(idx)
{
    // Width/height match visual size
    width  = (type == Type::PineTree) ? 18.0f : 22.0f;
    height = (type == Type::PineTree) ? 24.0f : 14.0f;
    position.x = worldX;
    position.y = LANE_GROUND_Y - height;

    if (lightCount > 0) {
        std::uniform_real_distribution<float> tangledDist(0.0f, 1.0f);
        float lightY = LANE_GROUND_Y - height * 0.5f;  // mid-height of object
        float segW   = width / static_cast<float>(lightCount);
        for (int i = 0; i < lightCount; ++i) {
            float lx = worldX + i * segW;
            bool tangled = tangledDist(s_bushRng) < tangledProb;
            auto ls = std::make_shared<LightString>(lx, lightY, segW - 1.0f, tangled, idx);
            m_lights.push_back(ls);
        }
    }
}

void BushTree::update(float dt) {
    for (auto& ls : m_lights) ls->update(dt);
}

void BushTree::render(SDL_Renderer* renderer, float cameraX) {
    float sx = position.x - cameraX;
    if (m_type == Type::PineTree)
        drawPineTree(renderer, sx);
    else
        drawBush(renderer, sx);
    for (auto& ls : m_lights) ls->render(renderer, cameraX);
}

void BushTree::drawBush(SDL_Renderer* renderer, float sx) const {
    // Alternate between two bush sprites for visual variety
    const char* sprite = (m_idx % 2 == 0) ? "bush_a" : "bush_b";
    SpriteRegistry::draw(renderer, sprite, sx, position.y, width, height);
}

void BushTree::drawPineTree(SDL_Renderer* renderer, float sx) const {
    SpriteRegistry::draw(renderer, "pine_small", sx, position.y, width, height);
}

}  // namespace LightsOut
