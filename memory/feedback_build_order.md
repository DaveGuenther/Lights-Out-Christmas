---
name: Build order — tests first
description: Always build and run unit tests before building the main game executable
type: feedback
---

Always run unit tests before building the main game binary.

**Why:** User explicitly requested this workflow so test failures are caught before producing a new game executable.

**How to apply:** When issuing build commands, build `LightsOutTests` first, run it, and only proceed to build `LightsOutChristmas` if all tests pass. The updated `build.sh` script does this automatically — prefer using it. When invoking cmake directly, sequence the targets: `--target LightsOutTests` → run tests → `--target LightsOutChristmas`.
