#pragma once
#include <string>
#include <vector>
#include <random>

namespace LightsOut {

struct HouseAsset {
    std::string name;
    std::string spritePath;  // relative to assets/
    std::string maskPath;    // relative to assets/
    float       pixelWidth;  // 256, 320, or 384
};

enum class HouseSize { Small, Medium, Large };

// Loads asset_data.json and provides random house selection by size category.
// Falls back to next-smaller category if requested size has no entries.
class HouseAssetLoader {
public:
    // basePath: the assets/ directory path (absolute)
    bool load(const std::string& basePath);

    // Select a random house asset for the given size.
    // Returns nullptr if no houses are available in any category.
    const HouseAsset* select(HouseSize size, std::mt19937& rng) const;

    bool isLoaded() const { return m_loaded; }

private:
    std::vector<HouseAsset> m_small;
    std::vector<HouseAsset> m_medium;
    std::vector<HouseAsset> m_large;
    bool m_loaded = false;

    const HouseAsset* selectFrom(const std::vector<HouseAsset>& v,
                                  std::mt19937& rng) const;
};

}  // namespace LightsOut
