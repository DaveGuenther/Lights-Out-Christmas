#pragma once
#include <string>
#include <cstdint>
#include <functional>

namespace LightsOut {

// Thin abstraction over the Steamworks SDK.
// When STEAM_ENABLED is not defined, all calls are no-ops and
// the game runs in "offline" mode (no Steam required).
class SteamIntegration {
public:
    static SteamIntegration& instance();

    bool init();
    void shutdown();
    void runCallbacks();   // call once per frame

    bool isAvailable() const { return m_available; }

    // ─── Leaderboards ───
    void submitScore(int score, std::function<void(bool success)> cb = {});
    void fetchLeaderboard(int maxEntries,
                          std::function<void(bool ok)> cb = {});

    struct LeaderboardEntry {
        std::string playerName;
        int         score;
        int         rank;
    };
    const std::vector<LeaderboardEntry>& leaderboardEntries() const {
        return m_entries;
    }

    // ─── Achievements ───
    void unlockAchievement(const char* apiName);
    bool hasAchievement(const char* apiName) const;

    // ─── Stats ───
    void setStatInt(const char* name, int value);
    void setStatFloat(const char* name, float value);
    void storeStats();   // flush to Steam

    // ─── Cloud save ───
    bool saveToCloud(const std::string& filename, const void* data, int size);
    bool loadFromCloud(const std::string& filename, void* outData, int maxSize,
                       int& outBytesRead);

private:
    SteamIntegration() = default;
    bool m_available = false;

    // Leaderboard handle (opaque; actual type depends on SDK)
    uint64_t m_leaderboardHandle = 0;

    std::vector<LeaderboardEntry> m_entries;

#ifdef STEAM_ENABLED
    // SDK-specific members would live here
#endif
};

}  // namespace LightsOut
