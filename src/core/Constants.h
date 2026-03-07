#pragma once
#include <cstdint>

namespace LightsOut {

// ─── Display ─────────────────────────────────────────────────────────────────
// Steam Deck native: 1280x800 (16:10)
// Pixel art base:    320x200 scaled 4x
inline constexpr int SCREEN_WIDTH  = 1280;
inline constexpr int SCREEN_HEIGHT = 800;
inline constexpr int RENDER_WIDTH  = 320;
inline constexpr int RENDER_HEIGHT = 200;
inline constexpr int PIXEL_SCALE   = 4;

// ─── Timing ──────────────────────────────────────────────────────────────────
inline constexpr float TARGET_FPS      = 60.0f;
inline constexpr float FIXED_TIMESTEP  = 1.0f / TARGET_FPS;  // seconds
inline constexpr int   MAX_FRAME_SKIP  = 5;

// ─── World / Scrolling ───────────────────────────────────────────────────────
inline constexpr float INITIAL_SCROLL_SPEED  = 45.0f;   // pixels/sec (render space)
inline constexpr float SCROLL_SPEED_INCREASE = 2.5f;    // per level
inline constexpr float MAX_SCROLL_SPEED      = 120.0f;

// ─── Lanes (Y = pixel position in render space, top-down) ────────────────────
// Lane 0: Rooftop  (y 95)  — squirrel runs along the roofline
// Lane 1: Fence    (y 128) — porch / fence level
// Lane 2: Ground   (y 160) — street / garden level
inline constexpr int   NUM_LANES          = 3;
inline constexpr float LANE_ROOFTOP_Y     = 95.0f;
inline constexpr float LANE_FENCE_Y       = 128.0f;
inline constexpr float LANE_GROUND_Y      = 160.0f;

// ─── Player ──────────────────────────────────────────────────────────────────
inline constexpr float PLAYER_START_X          = 50.0f;   // default screen X on spawn
inline constexpr float PLAYER_HORIZONTAL_SPEED = 55.0f;   // px/sec screen-space, left/right move
inline constexpr float PLAYER_SCREEN_X_MIN     = 20.0f;   // leftmost screen position allowed
inline constexpr float PLAYER_SCREEN_X_MAX     = 160.0f;  // rightmost screen position allowed
inline constexpr float PLAYER_SPEED            = 0.0f;    // world scrolls; player moves relative
inline constexpr float PLAYER_JUMP_SPEED  = 120.0f;  // vertical speed when jumping between lanes
inline constexpr float PLAYER_WIDTH       = 12.0f;
inline constexpr float PLAYER_HEIGHT      = 14.0f;
inline constexpr float PLAYER_BITE_RANGE  = 16.0f;   // horizontal range for biting lights
inline constexpr int   PLAYER_LIVES       = 3;
inline constexpr float PLAYER_RESPAWN_INVINCIBILITY = 5.0f;  // invincibility seconds on respawn
inline constexpr float RESPAWN_THREAT_FREEZE        = 5.0f;  // all threats frozen this long after respawn
inline constexpr float DEATH_OVERLAY_DURATION       = 2.0f;  // seconds to show death message before respawn

// ─── Scoring ─────────────────────────────────────────────────────────────────
inline constexpr int POINTS_PER_LIGHT           = 10;
inline constexpr int FULL_HOUSE_BONUS           = 500;
inline constexpr int FULL_HOUSE_MULTIPLIER      = 5;
inline constexpr int CHAIN_REACTION_BONUS       = 50;
inline constexpr int SPEED_BONUS_DIVISOR        = 100;   // score / time * divisor
inline constexpr int NEIGHBORHOOD_BONUS         = 2000;
inline constexpr float DARKNESS_FILL_PER_LIGHT  = 0.005f;
inline constexpr float DARKNESS_FILL_PER_HOUSE  = 0.05f;

// ─── House generation ────────────────────────────────────────────────────────
inline constexpr float HOUSE_MIN_GAP    = 20.0f;    // pixels between houses
inline constexpr float HOUSE_MAX_GAP    = 50.0f;
inline constexpr float HOUSE_MIN_WIDTH  = 55.0f;
inline constexpr float HOUSE_MAX_WIDTH  = 80.0f;
inline constexpr float HOUSE_HEIGHT     = 65.0f;
inline constexpr float HOUSE_GROUND_Y   = 165.0f;   // bottom of house

// ─── Light strings ───────────────────────────────────────────────────────────
inline constexpr float LIGHT_BULB_SIZE     = 3.0f;
inline constexpr float LIGHT_SPACING       = 8.0f;
inline constexpr int   LIGHT_BITES_TANGLED = 3;     // bites needed for tangled strands
inline constexpr float SPARK_DURATION      = 0.4f;  // seconds for the spark effect

// ─── Threats ─────────────────────────────────────────────────────────────────
inline constexpr float THREAT_CHASE_DURATION = 4.5f;  // seconds before giving up
inline constexpr float THREAT_FALL_ACCEL     = 220.0f; // pixels/sec² when falling
inline constexpr float HOMEOWNER_GRUMPY_SPEED  = 30.0f;
inline constexpr float HOMEOWNER_DAD_SPEED     = 55.0f;
inline constexpr float DOG_CHAIN_RADIUS        = 40.0f;
inline constexpr float CAT_SPEED              = 50.0f;
inline constexpr float OWL_PATIENCE_SECONDS   = 2.5f;   // time on branch before owl grabs
inline constexpr float WATCH_CAR_SPEED        = 20.0f;
inline constexpr float PORCH_LIGHT_RADIUS     = 35.0f;   // detection radius

// ─── Power-ups ───────────────────────────────────────────────────────────────
inline constexpr float POWERUP_DURATION_ACORN    = 5.0f;
inline constexpr float POWERUP_DURATION_COAT     = 6.0f;
inline constexpr float POWERUP_DURATION_SHADOW   = 7.0f;
inline constexpr float POWERUP_DURATION_CHOMP    = 4.0f;
inline constexpr float POWERUP_DURATION_ICE      = 4.0f;
inline constexpr float POWERUP_DURATION_DOUBLE   = 5.0f;
inline constexpr float POWERUP_DURATION_FRENZY   = 4.0f;
inline constexpr float ACORN_SPEED_MULTIPLIER    = 2.0f;
inline constexpr float FRENZY_SLOW_FACTOR        = 0.5f;  // world slows to this fraction

// ─── Particles ───────────────────────────────────────────────────────────────
inline constexpr int   SPARK_PARTICLE_COUNT = 8;
inline constexpr float SPARK_SPEED          = 30.0f;
inline constexpr int   SNOW_PARTICLES       = 60;
inline constexpr float SNOW_FALL_SPEED      = 12.0f;

// ─── Audio ───────────────────────────────────────────────────────────────────
inline constexpr int AUDIO_FREQUENCY     = 44100;
inline constexpr int AUDIO_CHANNELS      = 2;
inline constexpr int AUDIO_CHUNK_SIZE    = 2048;
inline constexpr int AUDIO_MIX_CHANNELS = 16;

// ─── UI ──────────────────────────────────────────────────────────────────────
inline constexpr int HUD_FONT_SIZE       = 16;
inline constexpr int MENU_FONT_SIZE      = 24;
inline constexpr int COMBO_FONT_SIZE     = 20;
inline constexpr float COMBO_DISPLAY_TIME = 1.5f;  // seconds

// ─── Upgrade costs ───────────────────────────────────────────────────────────
inline constexpr int UPGRADE_JUMP_HEIGHT_COST   = 30000;
inline constexpr int UPGRADE_BITE_SPEED_COST    = 35000;
inline constexpr int UPGRADE_MOVE_SPEED_COST    = 40000;
inline constexpr int UPGRADE_QUIET_STEPS_COST   = 50000;

// ─── Level count ─────────────────────────────────────────────────────────────
inline constexpr int NUM_LEVELS = 5;
// Level indices
inline constexpr int LEVEL_SUBURBAN     = 0;
inline constexpr int LEVEL_RICH         = 1;
inline constexpr int LEVEL_CULDESAC     = 2;
inline constexpr int LEVEL_CHRISTMAS_EVE = 3;
inline constexpr int LEVEL_TOWN_SQUARE  = 4;

}  // namespace LightsOut
