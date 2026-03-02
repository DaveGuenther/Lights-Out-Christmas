#pragma once
#include "core/Game.h"
#include "core/Types.h"
#include <vector>
#include <string>

namespace LightsOut {

class UpgradeScreen : public Screen {
public:
    UpgradeScreen(Game& game, int levelScore, int totalScore);

    void handleInput() override;
    void update(float dt) override;
    void render() override;

private:
    struct UpgradeOption {
        UpgradeType type;
        std::string name;
        std::string description;
        int         cost;
        int         currentLevel;
    };

    int   m_levelScore;
    int   m_totalScore;
    int   m_selected  = 0;
    int   m_spendable = 0;   // points available for upgrades
    float m_timer     = 0.0f;

    std::vector<UpgradeOption> m_options;

    void buildOptions();
    bool canAfford(const UpgradeOption& opt) const;
    void purchase(const UpgradeOption& opt);

    void drawBackground(SDL_Renderer* r) const;
    void drawTitle(SDL_Renderer* r) const;
    void drawOptions(SDL_Renderer* r) const;
    void drawBudget(SDL_Renderer* r) const;
    void drawSquirrelPreview(SDL_Renderer* r) const;
};

}  // namespace LightsOut
