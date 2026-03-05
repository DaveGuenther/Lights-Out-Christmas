# Lights Out Christmas — V1 Prompts

Prompts sent during the V1 (Atari-style generated graphics) development session.

---

## Session 1 — Initial Build

**1.** The current project is a 2D Pixel Art game called Lights Out Christmas. Details can be found in the Lights Out Christmas.txt file. Create agents as needed to complete the build process so that an executable can be created while all unittests successfully pass.

**2.** I have installed SDL dependencies through MSYS2 via pacman

**3.** but make sure that the CMakeList.txt can also run in Linux where SDL may be somewhere else

**4.** can you desribe the changes?

**5.** ok, continue

**6.** none of the input keys are responsive. UP/DOWN don't really work, SPACE isn't doing anything. My character can't move

**7.** Controls on the Main Menu don't work

**8.** How doees the squirrel attack the lights?

**9.** The bite action is not working in the game

---

## Session 2 — Houses, Threats, Scoring

**10.** How come there are no houses after the first two on level 1? After the squirrel passes the first two houses it's just empty after that. For a Suburban level, make each level have on average 50 houses. Easier levels should have more dark houses that have no lights, and harder levels should have more houses with lights

**11.** The plater isn't able to select the powerup at the end of the level by pressing Enter. The threats on screen don't do anything to the player when passing by. They should try to chase or fly after the player. There are still no dark houses, every house seems to have lights on the roof that can be eaten.

**12.** Threats chasing the player should stop after a few seconds and fall off screen. The squirrel shouldn't be able to fly, there should only be three "lanes" with it's highest point being the roof line. Right now there are only roof and window lights, add some bushes and trees on the ground level that the squirrel can munch. Keep the level difficulty instructions previously provided in place, but add these. The houses themselves should have difficulty levels, easy houses that aren't dark should have only one tier with lights: either the roof, window, or ground. Medium difficulty houses should have lights on two tiers that are next to eachother. Very difficult houses should have lights on all tiers. Earlier levels should have more easy houses, a few medium houses and maybe one hard house. Medium levels should have an even mix of house types. Hard levels should have mostly hard houses, a few medium houses and maybe one or two easy houses. Provide a running total of points accumulated at all times on the screen, and show it at the level end of the level when the player buys a power-up. At the end of the level, space bar shouldn't do anything on the purchase screen. Enter should make a purchase, ot escape should skip to the next level. points can accumulate across levels. Upgrades should stak on top of eachother.

---

## Session 3 — Tests, Lives, Lighting, Sprites

**13.** Are there any more unit tests that need to be added based on change implemented?

**14.** If ANY of the Threats collide with the squirrel, it should die. The squirrel gets 3 lives, starting each next life at the point that it previously died. Power Up items should cost at least 30000 points, so that the player gernerally can afford 1 or maybe 2 powerups. Shorted the level to 30 seconds. Add lighting effects to screen so that when light shines, the area around it is lighter, gradually darkening the further the light is from the screen.

**15.** I would like to update the graphics so that instead of generated graphics, I have sprites for all entities. Can you generate a list of all assets that would need sprites drawn, including other sprite are that can be used to fill the screen to make the neighborhoow look nice? Do not modify any code for this step, I just want a list of sprite assets that I should draw.

**16.** Can you provide a .md file with a complete list of the prompts I sent you in this chat? You don't need to include the attached documents, or your responses. I want to include this in my planning folder as a file called v1-atari-prompts.md
