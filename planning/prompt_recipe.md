# Lights Out Christmas — V1 Prompts (ATARI Style)

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

# Lights Out Christmas — V2 Prompts (NES Style)

---

## Session 4 — Using Claude to Generate Pixel Art

**16.** I have attached a concept document for Lights Out, Christmas!  It is a video game where you play as a squirrel trying to demolish the neighborhood lights display.  I have added a concept document as well as list of sprite assets that I want to generate to fit a common theme.  Can you generate all these sprites from sprite_asset_list.txt keeping in mind that this game is played at night? 

[Attached sprite_asset_list.txt and Lights Out Christmas Concept.txt1]
[Generated Lights out christmas sprites.jsx]

---

## Session 5 — Convert Procedural Art to Pixel Art

**17.** The JSX file you see contains sprites of assets that can be used in the Lights out, Christmas game. Look into the .JSX file and convert each sprite to it's named file as png files.

**18.** Modify all project files as necessary to replace procedurally generated images with the sprites in assets/sprites. Recompile and unit test as necessary.

---

## Session 6 — Boss Screen, Win Screen, Fixes, Menu & HUD

**19.** Continue from where you left off.

**20.** The game randomly froze while my squirrel was switching lanes and biting a bunch on the ground. Can you debug this error and then write a unit test to make sure it doesn't happen again?

**21.** At the end of the town square level, the player gets a 'Caught' message like the game is over, but actually the player needs to go to a final area where the town's main tree stands. The screen should stop scrolling once the tree is centered with many lights everywhere. The squirrel must demolish all the lights on the tree to win. While the squirrel is demolishing lights, owls and cats should be the main threats with dogs barking on the ground. Once the squirrel has demolished all the lights on the tree then the game should show a 'You win!' screen saying that you caused all the lights in the neighborhood to go out!

**22.** Add a selectable quit button on the menu screen and remove the leaderboard

**23.** can you make it so that each power up captured displays its effect on the screen in text for a few seconds?


