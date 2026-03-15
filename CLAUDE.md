# CLAUDE.md — Lights Out Christmas

Project-specific context and memory for Claude Code. Commit this file; it is
loaded automatically at the start of every session in this directory.

---

## Project Overview

**Lights Out Christmas** — a 2D side-scrolling pixel-art action game.
- Language: C++20
- Libraries: SDL2, SDL2_image, SDL2_mixer, SDL2_ttf, nlohmann/json, Catch2
- Toolchain: MSYS2 / clang++ on Windows, CMake + Ninja
- Render space: 640×400 scaled 2× to 1280×800 (Steam Deck native)
- Y=0 is top, Y=400 is bottom; `LANE_GROUND_Y=368`, `HOUSE_GROUND_Y=400`

---

## Build Workflow

**Always build and run unit tests before building the main executable.**

```bash
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:$PATH"
cd /c/coding/claude/Lights-Out-Christmas

# 1. Build + run tests first
cmake --build build-Release --target LightsOutTests
./build-Release/bin/LightsOutTests.exe

# 2. Only if tests pass — build the game
cmake --build build-Release --target LightsOutChristmas
```

Or just use `build.sh` which does this automatically.

**Note:** The post-build `cmd.exe` asset-copy step always fails in MSYS2 bash
("`'cmd.exe' is not recognized`") — this is a pre-existing issue and does NOT
mean the build failed. Verify success by checking the exe timestamp.

---

## Key Architecture

- `src/core/` — Game loop, input, audio, resource management, dev console
- `src/gameplay/` — World, player, houses, light strings, threats, power-ups
- `src/ui/` — Screens (MainMenu, GameScreen, PauseMenu, ControlsScreen, …)
- `src/rendering/` — Renderer wrapper, particles
- `assets/` — Sprites, masks, collision images, audio, `asset_data.json`

### Coordinate / camera model
- `cameraX` scrolls right over time; `screenX = worldX - cameraX`
- Player's `screenX` is canonical; `position.x = cameraX + screenX`
- Idle: squirrel drifts left on screen; at left wall it auto-runs

### Houses
- Small=256px, Medium=320px, Large=384px wide; all 320px tall (`HOUSE_HEIGHT`)
- Mask PNG colours: Blue→Rooftop lights, Yellow→Fence lights,
  Red→Ground lights, Green→Bush placement zones
- `parsePlatforms` scales pixel coords by `renderedWidth/imgWidth`

### Light strings
- `LightString::render()` skips fully-off strands (`isFullyOff()` guard)
- Each bulb renders: 4-layer soft glow orb + white-tinted core + sprite on top
- Bite detection: checks player X range overlaps strand `[x1, x2]`, not centre

### Darkness meter
- Fill rates (halved 2025-03): `DARKNESS_FILL_PER_LIGHT=0.000625`,
  `DARKNESS_FILL_PER_HOUSE=0.00625`
- When meter is full, `darkHouseProbability` is forced to 0 so new houses
  are always lit — player can keep scoring until level ends
- Ambient overlay capped at `min(darkness, 0.4) * 70 + 15` alpha

---

## Feedback / Preferences

- **Tests before build**: Always build `LightsOutTests` and run them before
  building `LightsOutChristmas`. Abort if any test fails.
- **Terse responses**: No trailing summaries; the diff speaks for itself.
- **No over-engineering**: Don't add features, helpers, or abstractions beyond
  what was asked.

---

## Memory Index

Additional memory files live in [`memory/`](memory/) inside this repo:

| File | Contents |
|------|----------|
| [feedback_build_order.md](memory/feedback_build_order.md) | Always run tests before building the main game |

