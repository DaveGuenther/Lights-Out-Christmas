#include "gameplay/House.h"
#include "core/Constants.h"
#include "core/SpriteRegistry.h"
#include <SDL2/SDL.h>
#include <random>
#include <algorithm>

namespace LightsOut {

static std::mt19937 s_houseRng(123);

House::House(float worldX, float worldWidth, HouseStyle style,
             int houseIndex, int roofStrands, int windowStrands, int porchStrands,
             float tangledProb)
    : Entity(TAG_HOUSE)
    , m_houseIndex(houseIndex)
    , m_style(style)
{
    position.x = worldX;
    position.y = HOUSE_GROUND_Y - HOUSE_HEIGHT;
    width  = worldWidth;
    height = HOUSE_HEIGHT;

    // Random palette per house
    static const Color walls[]  = {{50,50,65},{45,35,30},{30,50,45},{55,45,35}};
    static const Color roofs[]  = {{35,25,20},{25,35,45},{40,25,40},{30,40,25}};
    std::uniform_int_distribution<int> wd(0,3), rd(0,3);
    wallColor = walls[wd(s_houseRng)];
    roofColor = roofs[rd(s_houseRng)];

    if (roofStrands   > 0) placeRoofLights(roofStrands, tangledProb);
    if (windowStrands > 0) placeWindowLights(windowStrands, tangledProb);
    if (porchStrands  > 0) placePorchLights(porchStrands, tangledProb);
}

void House::update(float dt) {
    for (auto& ls : m_lightStrings) ls->update(dt);

    if (m_porchResetting) {
        m_porchResetTimer -= dt;
        if (m_porchResetTimer <= 0.0f) {
            m_porchLightOn  = false;
            m_porchResetting = false;
        }
    }
}

void House::render(SDL_Renderer* renderer, float cameraX) {
    float sx = position.x - cameraX;

    drawHouseBody(renderer, sx);
    drawPorchLight(renderer, sx);

    for (auto& ls : m_lightStrings) ls->render(renderer, cameraX);
}

bool House::isFullyDark() const {
    for (const auto& ls : m_lightStrings)
        if (!ls->isFullyOff()) return false;
    return !m_lightStrings.empty();
}

Vec2 House::porchLightPos() const {
    return {position.x + width * 0.15f, HOUSE_GROUND_Y - 15.0f};
}

void House::triggerPorchLight() {
    m_porchLightOn  = true;
    m_porchResetting = false;
}

void House::resetPorchLight(float delaySeconds) {
    m_porchResetTimer = delaySeconds;
    m_porchResetting  = true;
}

void House::placeRoofLights(int count, float tangledProb) {
    if (count <= 0) return;
    std::uniform_real_distribution<float> tangledDist(0.0f, 1.0f);

    float segW = width / static_cast<float>(count);
    for (int i = 0; i < count; ++i) {
        float lx = position.x + static_cast<float>(i) * segW;
        float ly = position.y + 2.0f;  // along roofline
        bool tangled = tangledDist(s_houseRng) < tangledProb;
        auto ls = std::make_shared<LightString>(lx, ly, segW - 2.0f, tangled, m_houseIndex);
        m_lightStrings.push_back(ls);
    }
}

void House::placeWindowLights(int count, float tangledProb) {
    if (count <= 0) return;
    std::uniform_real_distribution<float> tangledDist(0.0f, 1.0f);

    // Window-ledge lights sit just below the window row (~20px below house top)
    float windowY = position.y + 22.0f;
    float segW    = width / static_cast<float>(count);
    for (int i = 0; i < count; ++i) {
        float lx = position.x + static_cast<float>(i) * segW;
        bool tangled = tangledDist(s_houseRng) < tangledProb;
        auto ls = std::make_shared<LightString>(lx, windowY, segW - 2.0f, tangled, m_houseIndex);
        m_lightStrings.push_back(ls);
    }
}

void House::placePorchLights(int count, float tangledProb) {
    if (count <= 0) return;
    std::uniform_real_distribution<float> tangledDist(0.0f, 1.0f);

    float porchY  = HOUSE_GROUND_Y - 22.0f;
    float porchW  = width * 0.6f;
    float startX  = position.x + width * 0.2f;
    float segW    = porchW / static_cast<float>(count);

    for (int i = 0; i < count; ++i) {
        float lx = startX + static_cast<float>(i) * segW;
        bool tangled = tangledDist(s_houseRng) < tangledProb;
        auto ls = std::make_shared<LightString>(lx, porchY, segW - 2.0f, tangled, m_houseIndex);
        m_lightStrings.push_back(ls);
    }
}

void House::drawHouseBody(SDL_Renderer* renderer, float screenX) const {
    // Pick wall / roof / door variants from house index for visual variety
    static const char* wallSprites[] = {"house_wall_a", "house_wall_b", "house_wall_c"};
    static const char* roofSprites[] = {"house_roof_a", "house_roof_b", "house_roof_c"};
    static const char* doorSprites[] = {"house_door_a", "house_door_b"};

    int wallVar = m_houseIndex % 3;
    int roofVar = (m_houseIndex / 2) % 3;
    int doorVar = m_houseIndex % 2;

    // Roof (18 px tall, overhangs wall by 2 px each side)
    float roofW = width + 4.0f;
    float roofX = screenX - 2.0f;
    float roofY = position.y;
    SpriteRegistry::draw(renderer, roofSprites[roofVar], roofX, roofY, roofW, 18.0f);

    // Snow cap on top of roof
    SpriteRegistry::draw(renderer, "house_snow_cap", roofX, roofY - 2.0f, roofW, 6.0f);

    // Wall (below roof)
    float wallY = position.y + 18.0f;
    float wallH = height - 18.0f;
    SpriteRegistry::draw(renderer, wallSprites[wallVar], screenX, wallY, width, wallH);

    // Icicles along roofline
    SpriteRegistry::draw(renderer, "house_icicles", screenX, roofY + 16.0f, width, 6.0f);

    // Chimney (natural size 8x14)
    SpriteRegistry::draw(renderer, "house_chimney",
                         screenX + width * 0.75f, roofY - 10.0f);

    // Windows — lit or dark based on whether porch light (or any light) is on
    const char* winSprite = m_porchLightOn ? "house_window" : "house_window";
    SpriteRegistry::draw(renderer, winSprite,
                         screenX + width * 0.15f, wallY + 4.0f);
    SpriteRegistry::draw(renderer, winSprite,
                         screenX + width * 0.60f, wallY + 4.0f);

    // Door (natural size 12x20)
    SpriteRegistry::draw(renderer, doorSprites[doorVar],
                         screenX + width * 0.42f, position.y + height - 22.0f);
}

void House::drawPorchLight(SDL_Renderer* renderer, float screenX) const {
    float lpx = screenX + width * 0.15f;
    float lpy = HOUSE_GROUND_Y - 15.0f;

    // Porch light sprite (8x10, center-left aligned)
    const char* lightSprite = m_porchLightOn ? "house_porch_light_on" : "house_porch_light_off";
    SpriteRegistry::draw(renderer, lightSprite, lpx - 4.0f, lpy - 2.0f);

    // When on, add a soft warm cone below (procedural, dynamic effect)
    if (m_porchLightOn) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        for (int i = 0; i < 5; ++i) {
            float fi = static_cast<float>(i);
            SDL_SetRenderDrawColor(renderer, 255, 230, 120,
                                   static_cast<uint8_t>(50 - i * 8));
            SDL_FRect cone = {lpx - fi * 2.5f, lpy + fi * 4.0f,
                              fi * 5.0f + 3.0f, 4.0f};
            SDL_RenderFillRectF(renderer, &cone);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

}  // namespace LightsOut
