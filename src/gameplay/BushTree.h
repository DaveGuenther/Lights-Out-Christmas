#pragma once
#include "Entity.h"
#include "LightString.h"
#include <vector>
#include <memory>

namespace LightsOut {

// A small decoratable ground-level object (bush or pine tree) that can
// have Christmas lights on it for the squirrel to bite.
class BushTree : public Entity {
public:
    enum class Type { Bush, PineTree };

    BushTree(float worldX, Type type, int lightCount, float tangledProb, int idx);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, float cameraX) override;

    std::vector<std::shared_ptr<LightString>>& lightStrings() { return m_lights; }
    int objectIndex() const { return m_idx; }

private:
    Type  m_type;
    int   m_idx;
    std::vector<std::shared_ptr<LightString>> m_lights;

    void drawBush(SDL_Renderer* renderer, float sx) const;
    void drawPineTree(SDL_Renderer* renderer, float sx) const;
};

}  // namespace LightsOut
