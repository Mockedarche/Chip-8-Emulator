# Chip 8 Emulator implemented in C with raylib

Supports custom speed, input, sound, graphics (with customization for scale and colors), debugging output along with ability to walkthrough each instruction with option print command to see all the chip 8s internal memories, AND ability to change any chip 8 quirk on and off by just passing a flag.

with all the features done and even quirks customizable for the given rom this project is considered launch. It's fully functional, cusotmizable, stable, and most bugs have been caught and fixed (as much as I can find playing various games). 

Tested on M1 mac running macos taho 26.01
Note the makefile was included as an example of how to build it raylib is of course a requirement I used 5.5.0 for development. Other than that its all standard C.

Help menu 
Expected behavior is ./chip_8_emulator arguments path_to_ch8_rom
arguments are -BGCOLOR = any raylib color, -PCOLOR = any raylib color
-SPEED=float, -SCALE_FACTOR=int, -debug=bool, -walkthrough=bool 
-vf_reset=bool, -memory_quirk=bool, -display_wait=bool, -clipping_quirk=bool, shifting_quirk=bool, -jumping_quirk=bool
Available colors are: darkgray, maroon, orange, darkgreen, darkblue, darkpurple, darkbrown, gray, red, gold, lime, blue, violet, brown, lightgray, pink, yellow, green, skyblue, purple, beige, black, white

Performance wise im sure it could be faster but generally 660 instructions per second is considered real time but uncapped my M1 mac could
run at ~330,000 instructions per second which is definitely crazy fast.

Made this to get into emulators (software emulation of hardware), review on my C (forgot how much I love C) after a long period of python, and it just seemed enjoyable which it really was. 

Used [chip 8 test roms](https://github.com/Timendus/chip8-test-suite?tab=readme-ov-file) 
to test the emulator for accuracy and quirks

IBM test (bit accurate) with various colors
![](https://github.com/Mockedarche/Chip-8-Emulator/blob/main/Media/color_example.gif?raw=true)

Corax + opcode test (all passed)
![](https://github.com/Mockedarche/Chip-8-Emulator/blob/main/Media/Corax+opcode_test.png?raw=true)

Flags test (all passed)
![](https://github.com/Mockedarche/Chip-8-Emulator/blob/main/Media/flags_test.png?raw=true)

Input test (correctly gets input)
![](https://github.com/Mockedarche/Chip-8-Emulator/blob/main/Media/input_test.png)

Quirks test (none failed on or off just indicates behavior)
![](https://github.com/Mockedarche/Chip-8-Emulator/blob/main/Media/quirks_test.png?raw=true)

Gif of [chip 8 rom](https://johnearnest.github.io/chip8Archive/play.html?p=1dcell) Running at 10x real time speed
![](https://github.com/Mockedarche/Chip-8-Emulator/blob/main/Media/10Xrealtime.gif?raw=true)

