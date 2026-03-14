#pragma once
#include <string>
#include <vector>
#include <random>

namespace LightsOut {

struct HouseAsset {
    std::string name;
    std::string spritePath;     // absolute path
    std::string maskPath;       // absolute path
    std::string collisionPath;  // absolute path
    float       pixelWidth;     // 256, 320, or 384
};

struct TreeAsset {
    std::string name;
    std::string spritePath;     // absolute path
    std::string maskPath;       // absolute path
    std::string collisionPath;  // absolute path
    float       pixelWidth;
    float       pixelHeight;
};

// Bush assets follow the same layout as tree assets
using BushAsset = TreeAsset;

enum class HouseSize { Small, Medium, Large };

// Loads asset_data.json and provides random house/tree selection.
class HouseAssetLoader {
public:
    // basePath: the assets/ directory path (absolute)
    bool load(const std::string& basePath);

    // Select a random house asset for the given size category.
    // Falls back to next-smaller category if the requested one is empty.
    // Returns nullptr if no houses are available at all.
    const HouseAsset* select(HouseSize size, std::mt19937& rng) const;

    // Select a random tree asset from the small/med_small/med_large pool.
    // Returns nullptr if no trees were loaded.
    const TreeAsset* selectTree(std::mt19937& rng) const;
    const BushAsset* selectBush(std::mt19937& rng) const;

    const std::vector<BushAsset>& bushAssets() const { return m_bushes; }

    bool isLoaded() const { return m_loaded; }

private:
    std::vector<HouseAsset> m_small;
    std::vector<HouseAsset> m_medium;
    std::vector<HouseAsset> m_large;

    std::vector<TreeAsset>  m_trees;   // small + med_small + med_large combined
    std::vector<BushAsset>  m_bushes;  // from "bushes" section

    bool m_loaded = false;

    const HouseAsset* selectFrom(const std::vector<HouseAsset>& v,
                                  std::mt19937& rng) const;
};

}  // namespace LightsOut
