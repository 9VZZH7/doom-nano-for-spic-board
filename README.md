# doom-nano-for-spic-board
A 3d raycast engine for the SPiC-Board. The implementation is based on daveruiz doom-nano. (https://github.com/daveruiz/doom-nano)

See the original implementation in action:
- https://youtu.be/wJG04DSo7Xw (Progress on Feb, 09 2019)
- https://youtu.be/FX7PQtUAkHk (Progress on Jan, 18 2019)

Like the the original implementation, this project can be run on a ATMega328PB, this hardware is very limited in power and memory: 16MHz, only 32kB for program memory and 2kB of RAM, of which 1kB is entirely used for the screen. Additionaly there is 1kB of EEPROM present, which will be used to store levels to make more room for the implementation of features.
Despite the original implementation running at 14 FPS, I struggle to have it run consistent. This might be because I decided to switch the programming language to C, or because I use a different library for the display.

As the original author said:
To be clear. **This is not an actual Doom game**, just picked some sprites from it (and simplified a lot). The rendering engine is more like a Wolfeistein 3D. The Doom idea came because I started this building the fancy melt-screen effect (included in an early version, but not anymore).

I started porting this to the SPiC-Board because of a course at FAU.

Hardware I used:
- A SPiC-Board (You have to take the course Systemnahe Programmierung in C at FAU to get one, this might be impossible if you dont want to study informatics in Germany)

Resources:
- Sprites from https://www.spriters-resource.com
- Much thanks to https://lodev.org/cgtutor for so wonderful resource about raycasting engines
- libspicboard provided by SPiC is needed to run this.

Current status: (Made in the original implementation)
- The map rendering is working nicely. Even I was able to add a depth effect by using different dithering patterns, depending on the distance to the view.
- Sprites are working too, though has some issues hiding them behind walls because memory limitations (the z-buffer precision has been limited a lot to make it smaller).
- You can move through the map, collide with walls, collect items and interact with enemies. I could also add the jogging effect, like Doom´s one.
- The enemies AI, despite is very simple, I think works very well and it´s enough for the purpose of the game. Looks very similar to  Imp enemy from original Doom.
- For the HUD, I realized that the native `print` from Adafruit's library uses too much memory. So I've implemented my custom text rendering methods and a custom font which includes only needed characters and some icons.
- FPS and memory consumption has been improved using a slim and customized version of the SSD1306 lib from Adafruit. Thanks  [@miracoly](https://github.com/miracoli) -> Which does not hold true for my implementation
- Now it has a basic sound support through Pin 9. Thanks again [@miracoly](https://github.com/miracoli)! -> Since the SPiC-Board has no sound output, I did cut this feature

(I'd like) To do:
- Doors and locked doors.
- A game over screen.
- Get the start screen working.
- Maybe mdd more sprites, decorative elements, etc.
- Save one or mor levels to EEPROM memory, this will be quite hard, one level currently uses around 1.8kB of memory. The EEPROM has a space of 1kB.
