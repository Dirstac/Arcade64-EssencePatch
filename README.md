# Arcade64-EssencePatch
What is Arcade64 Essence Patch?

This is a version that implements some features that the [PSARCADECLASSIC+](https://github.com/Gaston900/PSArcadeClassic) emulator had, to the most recent versions of [ARCADE64](https://arcade.mameworld.info/).

What has been optimized in this version?
---------------------------------------
This command will help us have much easier access to the settings:

* To enter the BIOS, press the "0" key.

Players 1 and 2 use this controller by default:

* To move, use the arrow keys "Up, Down, Left, Right."
* To perform actions, use the "A, S, D, Z" keys.

Added custom buttons and autofire.

Removed warning screen, startups, Decryption screen texts.

Removed the following annoying messages: “WRONG LENGTH”, “NO_DUMP”, “WRONG CHECKSUMS”, “BAD_DUMP”.

Added movement optimization for all fighting games (By GSC2007).

The Arcade64 "GUI" EKMAME source code has been implemented (By KAZE).

The Arcade64 "GUI" some default settings have been implemented.

It is already pre-configured, enforce aspect ratio is disabled, full screen mode and tricks are already enabled by default.

The cheat reference function, if the cloned game does not have a cheat file, it will reference the cheat file of the main ROM.

Supports game list lst language files. The list language can be edited freely.

NEOGEO, PGM, driver supports key combination settings.

Added 13 new UI folders by default.
 * Capcom
 * Dataeast
 * IGS
 * konami
 * Namco
 * Nintendo
 * Neogeo
 * Midway
 * Sega
 * Seta
 * Taito
 * Toaplan
 * Misc

How to compile
---------------------------------------
In order to compile this version we will need to download the [ARCADE64](https://github.com/Robbbert/abcdefg/tags) source codes. How do we know which version we need? We will have to locate the latest compilation that I have released publicly.

```
make OSD=winui PTR64=1 SUBTARGET=arcade SYMBOLS=0 NO_SYMBOLS=1 DEPRECATED=0
```

The compilation [TOOL](https://github.com/mamedev/buildtools/releases) is suggested to be version msys64-2022-01-12.
