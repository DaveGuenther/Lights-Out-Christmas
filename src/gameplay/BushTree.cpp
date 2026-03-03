#include "gameplay/BushTree.h"
#include "core/Constants.h"
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
    // Three overlapping dark-green rounded blobs
    SDL_SetRenderDrawColor(renderer, 25, 70, 25, 255);
    SDL_FRect b1 = {sx,          position.y + 4.0f, 12.0f, 10.0f};
    SDL_FRect b2 = {sx + 7.0f,  position.y + 2.0f, 12.0f, 10.0f};
    SDL_FRect b3 = {sx + 3.0f,  position.y,         14.0f,  9.0f};
    SDL_RenderFillRectF(renderer, &b1);
    SDL_RenderFillRectF(renderer, &b2);
    SDL_RenderFillRectF(renderer, &b3);
    // Snow dusting
    SDL_SetRenderDrawColor(renderer, 210, 230, 255, 180);
    SDL_FRect snow = {sx + 2.0f, position.y, 18.0f, 3.0f};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRectF(renderer, &snow);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void BushTree::drawPineTree(SDL_Renderer* renderer, float sx) const {
    // Three triangular tiers from bottom to top
    SDL_SetRenderDrawColor(renderer, 20, 80, 30, 255);
    // Bottom tier (widest)
    SDL_FRect t1 = {sx,         position.y + 16.0f, 18.0f, 8.0f};
    // Middle tier
    SDL_FRect t2 = {sx + 3.0f,  position.y + 9.0f,  12.0f, 9.0f};
    // Top tier
    SDL_FRect t3 = {sx + 6.0f,  position.y + 2.0f,   6.0f, 9.0f};
    SDL_RenderFillRectF(renderer, &t1);
    SDL_RenderFillRectF(renderer, &t2);
    SDL_RenderFillRectF(renderer, &t3);
    // Trunk
    SDL_SetRenderDrawColor(renderer, 80, 50, 20, 255);
    SDL_FRect trunk = {sx + 7.0f, position.y + 22.0f, 4.0f, 2.0f};
    SDL_RenderFillRectF(renderer, &trunk);
    // Snow on tiers
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 210, 230, 255, 160);
    SDL_FRect s1 = {sx,         position.y + 16.0f, 18.0f, 2.0f};
    SDL_FRect s2 = {sx + 3.0f,  position.y + 9.0f,  12.0f, 2.0f};
    SDL_FRect s3 = {sx + 6.0f,  position.y + 2.0f,   6.0f, 2.0f};
    SDL_RenderFillRectF(renderer, &s1);
    SDL_RenderFillRectF(renderer, &s2);
    SDL_RenderFillRectF(renderer, &s3);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

}  // namespace LightsOut
