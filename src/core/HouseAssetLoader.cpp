#include "core/HouseAssetLoader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace LightsOut {

static void loadHouseCategory(const nlohmann::json& arr,
                               std::vector<HouseAsset>& out,
                               float width,
                               const std::string& basePath)
{
    for (const auto& entry : arr) {
        HouseAsset a;
        a.name          = entry.value("name",      "");
        a.spritePath    = basePath + "/" + entry.value("sprite",    "");
        a.maskPath      = basePath + "/" + entry.value("mask",      "");
        a.collisionPath = basePath + "/" + entry.value("collision", "");
        a.pixelWidth    = width;
        if (!a.name.empty()) out.push_back(std::move(a));
    }
}

static void loadTreeCategory(const nlohmann::json& arr,
                              std::vector<TreeAsset>& out,
                              const std::string& basePath)
{
    for (const auto& entry : arr) {
        TreeAsset a;
        a.name          = entry.value("name",      "");
        a.spritePath    = basePath + "/sprites/" + entry.value("sprite",    "");
        a.maskPath      = basePath + "/sprites/" + entry.value("mask",      "");
        a.collisionPath = basePath + "/sprites/" + entry.value("collision", "");
        a.pixelWidth    = 0.0f;
        a.pixelHeight   = 0.0f;

        // Probe actual sprite dimensions
        if (!a.spritePath.empty()) {
            SDL_Surface* s = IMG_Load(a.spritePath.c_str());
            if (s) {
                a.pixelWidth  = static_cast<float>(s->w);
                a.pixelHeight = static_cast<float>(s->h);
                SDL_FreeSurface(s);
            }
        }

        if (!a.name.empty()) out.push_back(std::move(a));
    }
}

bool HouseAssetLoader::load(const std::string& basePath) {
    std::string jsonPath = basePath + "/asset_data.json";
    std::ifstream f(jsonPath);
    if (!f.is_open()) return false;

    nlohmann::json j;
    try { f >> j; } catch (...) { return false; }

    if (j.contains("small_house"))
        loadHouseCategory(j["small_house"],  m_small,  256.0f, basePath);
    if (j.contains("medium_house"))
        loadHouseCategory(j["medium_house"], m_medium, 320.0f, basePath);
    if (j.contains("large_house"))
        loadHouseCategory(j["large_house"],  m_large,  384.0f, basePath);

    // Load trees: small, med_small, med_large  (NOT tree_large)
    for (const char* cat : {"tree_small", "tree_med_small", "tree_med_large"}) {
        if (j.contains(cat)) loadTreeCategory(j[cat], m_trees, basePath);
    }

    // Load bushes
    if (j.contains("bushes")) loadTreeCategory(j["bushes"], m_bushes, basePath);

    m_loaded = true;
    return true;
}

const HouseAsset* HouseAssetLoader::selectFrom(const std::vector<HouseAsset>& v,
                                                std::mt19937& rng) const
{
    if (v.empty()) return nullptr;
    std::uniform_int_distribution<size_t> dist(0, v.size() - 1);
    return &v[dist(rng)];
}

const HouseAsset* HouseAssetLoader::select(HouseSize size, std::mt19937& rng) const {
    if (size == HouseSize::Large) {
        if (auto* a = selectFrom(m_large,  rng)) return a;
        if (auto* a = selectFrom(m_medium, rng)) return a;
        return selectFrom(m_small, rng);
    }
    if (size == HouseSize::Medium) {
        if (auto* a = selectFrom(m_medium, rng)) return a;
        return selectFrom(m_small, rng);
    }
    return selectFrom(m_small, rng);
}

const TreeAsset* HouseAssetLoader::selectTree(std::mt19937& rng) const {
    if (m_trees.empty()) return nullptr;
    std::uniform_int_distribution<size_t> dist(0, m_trees.size() - 1);
    return &m_trees[dist(rng)];
}

const BushAsset* HouseAssetLoader::selectBush(std::mt19937& rng) const {
    if (m_bushes.empty()) return nullptr;
    std::uniform_int_distribution<size_t> dist(0, m_bushes.size() - 1);
    return &m_bushes[dist(rng)];
}

}  // namespace LightsOut
