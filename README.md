# Chip 8 Emulator implemented in C with raylib

Supports various speeds, input, sound, graphics, and with many more customization coming soon.

Performance wise im sure it could be faster but generally 660 instructions per second is considered real time but uncapped my M1 mac could
run at ~330,000 instructions per second which is definitely crazy fast.

Made this to get into emulators (software emulation of hardware), review on my C (forgot how much I love C) after a long period of python, and it just seemed enjoyable which it really was. 

Note the makefile was included as an example of how to build it raylib is of course a requirement I used 5.5.0 for development. Other than that its all 
standard C.

Used [chip 8 test roms](https://github.com/Timendus/chip8-test-suite?tab=readme-ov-file) 
to test the emulator for accuracy and quirks

IBM test (bit accurate)
![](https://github.com/Mockedarche/Chip-8-Emulator/blob/main/Media/IBM_test.png?raw=true)

Flags test (all passed)
![](https://github.com/Mockedarche/Chip-8-Emulator/blob/main/Media/flags_test.png?raw=true)

Input test (correctly gets input)
![](https://github.com/Mockedarche/Chip-8-Emulator/blob/main/Media/input_test.png)

Quirks test (none failed on or off just indicates behavior)
![](https://github.com/Mockedarche/Chip-8-Emulator/blob/main/Media/quirks_test.png?raw=true)

Gif of [chip 8 rom](https://johnearnest.github.io/chip8Archive/play.html?p=1dcell) Running at roughly 10x real time speed
![](https://github.com/Mockedarche/Chip-8-Emulator/blob/main/Media/10Xrealtime.gif?raw=true)

