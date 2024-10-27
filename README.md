# Arcade64-EssencePatch
What is Arcade64 Essence Patch?

This is a version that implements some features that the [PSARCADECLASSIC+](https://github.com/Gaston900/PSArcadeClassic) emulator had, to the most recent versions of Arcade64.

What has been optimized in this version?
---------------------------------------
The command of all the game systems was optimized, now it is much more practical to be able to play, the command of the arrow keys "Up, Down, Left, Right" comes by default, this command is used to be able to move and to take actions For shooting or jumping and throwing grenades, the A, S, D key is used and the rest of the keys are used for certain kinds of games Z, X, C, Q, W, E.

Added custom buttons and autofire.

Removed warning screen, startups, Decryption screen texts.

Removed the following annoying messages: “WRONG LENGTH”, “NO_DUMP”, “WRONG CHECKSUMS”, “BAD_DUMP” And “THE PARENT FILE DOES NOT EXIST”.

Added movement optimization for all fighting games (By GSC2007).

The Arcade64 “GUI” was modernized with new icons (By KOF2112/EKMAME), a custom default configuration has been made.

It is already pre-configured, enforce aspect ratio is disabled, full screen mode and tricks are already enabled by default.

The cheat reference function, if the cloned game does not have a cheat file, it will reference the cheat file of the main ROM.

Supports game list lst language files. The list language can be edited freely.

NEOGEO, PGM, driver supports key combination settings.

Added 12 new UI folders by default.
 * Capcom
 * Dataeast
 * IGS
 * konami
 * Namco
 * Neogeo
 * Midway
 * Sega
 * Taito
 * Seta
 * Toaplan
 * Misc

How to compile
---------------------------------------
In order to compile this version we will need to download the [MAME](https://github.com/mamedev/mame/releases) source codes. How do we know which version we need? We will have to locate the latest compilation that I have released publicly.

```
make OSD=winui PTR64=1 SUBTARGET=arcade SYMBOLS=0 NO_SYMBOLS=1 DEPRECATED=0
```

The compilation [TOOL](https://github.com/mamedev/buildtools/releases) is suggested to be version msys64-2022-01-12.
