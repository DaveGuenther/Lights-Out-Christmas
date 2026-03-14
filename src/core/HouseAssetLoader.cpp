#include "core/HouseAssetLoader.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace LightsOut {

static void loadCategory(const nlohmann::json& arr,
                         std::vector<HouseAsset>& out,
                         float width,
                         const std::string& basePath)
{
    for (const auto& entry : arr) {
        HouseAsset a;
        a.name        = entry.value("name",   "");
        a.spritePath  = basePath + "/" + entry.value("sprite", "");
        a.maskPath    = basePath + "/" + entry.value("mask",   "");
        a.pixelWidth  = width;
        if (!a.name.empty()) out.push_back(std::move(a));
    }
}

bool HouseAssetLoader::load(const std::string& basePath) {
    std::string jsonPath = basePath + "/asset_data.json";
    std::ifstream f(jsonPath);
    if (!f.is_open()) return false;

    nlohmann::json j;
    try {
        f >> j;
    } catch (...) {
        return false;
    }

    if (j.contains("small_house"))
        loadCategory(j["small_house"],  m_small,  256.0f, basePath);
    if (j.contains("medium_house"))
        loadCategory(j["medium_house"], m_medium, 320.0f, basePath);
    if (j.contains("large_house"))
        loadCategory(j["large_house"],  m_large,  384.0f, basePath);

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
    // Try requested size, then fall back to smaller categories
    if (size == HouseSize::Large) {
        if (auto* a = selectFrom(m_large, rng))  return a;
        if (auto* a = selectFrom(m_medium, rng)) return a;
        return selectFrom(m_small, rng);
    }
    if (size == HouseSize::Medium) {
        if (auto* a = selectFrom(m_medium, rng)) return a;
        return selectFrom(m_small, rng);
    }
    return selectFrom(m_small, rng);
}

}  // namespace LightsOut
