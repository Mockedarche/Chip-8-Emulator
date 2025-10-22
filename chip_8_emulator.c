/*
 * Chip 8 Emulator built by Austin Lunbeck (Mockedarche)
 * All in straight C with raylib (common crossplatform graphics, audio, input etc framework)
 * Only external library used was raylib rest was standard lubrary C (with POSIX header for time)
 *
 * Supports all opcodes, graphics, sound, input (qwerty), scaler for graphics, customization features for output and audio file (flags)
 *
 * Mean, Lean, Chip 8 Machine
 *
 * Requirements for build is raylib used version 5.5.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "timing.h"
#include "chip_8_core.h"
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>

// DEBUG FLAG
bool debug = false;
bool walk_through_each_instruction = false;

// Flag for flag register to be reset by 8xy1, 8xy2, 8xy3
bool vf_reset_quirk = true;
// Flag for Fx55 and Fx65 to increment index register
bool memory_quirk = true;
// Flag for capping drawing sprites to 60hz (simplification)
bool display_wait_quirk = true;
// Flag to enable sprites to clip instead of wrap
bool clipping_quirk = true;
// Flag for 8xy6 and 8xyE to shift V[y] placed into V[x] or just shift V[x] inplace
bool shifting_quirk = false;
// Flag for 1nnn to jump to nnn + V[n top nibble] + nnn or nnn + V[0]
bool jumping_quirk = false;

/*
 * draw_frame function
 * Expects: a pointer to a chip_8 struct
 * Does: Draws the current state of the CHIP-8 display to the raylib window
 *
 */
int draw_frame(chip_8 *chip_8_object, int scale_factor, Color *primary, Color *background){

    for (int i = 0; i < chip_8_screen_width; i++){
        for (int j = 0; j < chip_8_screen_height; j++){
            if (chip_8_object->display[j * chip_8_screen_width + i]){
                DrawRectangle(i * scale_factor, j * scale_factor, scale_factor, scale_factor, *primary);
            }
            else {
                DrawRectangle(i * scale_factor, j * scale_factor, scale_factor, scale_factor, *background);
            }
        }
    }

    chip_8_object->display_has_changed = false;
    return 0;

}

/*
 * get_most_recent_input function
 * Expects: chip_8_object to be correctly initialized
 * Does: Returns which character has been pressed most recently (cap 200 ms old) or 0xFF as none were pressed
 *
 */
u8 get_most_recent_input(chip_8 *chip_8_object) {
    // max age in ms
    int most_recent_age = 20;
    // default if no key is recent
    u8 recent_key = 0xFF;
    struct timespec temp;

    for (int i = 0; i < 16; i++) {
        int age = millis_since(chip_8_object->when_key_last_pressed[i]);
        if (age < most_recent_age) {
            most_recent_age = age;
            recent_key = i;
        }
    }
    return recent_key;

}

/*
 * get_color_from_name function
 * Expects: NA
 * Does: Given a string representation of a raylib defined color it returns that raylib color
 *
 */
Color get_color_from_name(const char *color_name){

    // Assuming user gave a recognized color return its Color type representation default to white
    if (strcasecmp(color_name, "darkgray") == 0) return DARKGRAY;
    else if (strcasecmp(color_name, "maroon") == 0) return MAROON;
    else if (strcasecmp(color_name, "orange") == 0) return ORANGE;
    else if (strcasecmp(color_name, "darkgreen") == 0) return DARKGREEN;
    else if (strcasecmp(color_name, "darkblue") == 0) return DARKBLUE;
    else if (strcasecmp(color_name, "darkpurple") == 0) return DARKPURPLE;
    else if (strcasecmp(color_name, "darkbrown") == 0) return DARKBROWN;
    else if (strcasecmp(color_name, "gray") == 0) return GRAY;
    else if (strcasecmp(color_name, "red") == 0) return RED;
    else if (strcasecmp(color_name, "gold") == 0) return GOLD;
    else if (strcasecmp(color_name, "lime") == 0) return LIME;
    else if (strcasecmp(color_name, "blue") == 0) return BLUE;
    else if (strcasecmp(color_name, "violet") == 0) return VIOLET;
    else if (strcasecmp(color_name, "brown") == 0) return BROWN;
    else if (strcasecmp(color_name, "lightgray") == 0) return LIGHTGRAY;
    else if (strcasecmp(color_name, "pink") == 0) return PINK;
    else if (strcasecmp(color_name, "yellow") == 0) return YELLOW;
    else if (strcasecmp(color_name, "green") == 0) return GREEN;
    else if (strcasecmp(color_name, "skyblue") == 0) return SKYBLUE;
    else if (strcasecmp(color_name, "purple") == 0) return PURPLE;
    else if (strcasecmp(color_name, "beige") == 0) return BEIGE;
    else if (strcasecmp(color_name, "black") == 0) return BLACK;
    return WHITE;

}

/*
 * main function
 * Expects: one argument: path to the CHIP-8 ROM file
 * Does: Initializes the CHIP-8 emulator, loads the ROM into memory, and starts the emulation loop.
 *
 */
int main(int argc, const char * argv[]) {

    /* Set up variables for emulation such as arguments */

    // Declare and init our chip_8 struct (blanking it to 0 to prevent bad data)
    chip_8 chip_8_instance;
    memset(&chip_8_instance, 0, sizeof(chip_8_instance));

    // Our Emulated stack requires top be init to -1
    chip_8_instance.emulated_stack.top = -1;
    // Last update needs to be assigned a value at start up
    chip_8_instance.last_update = current_ms();
    // Point out program counter to where the rom starts
    chip_8_instance.PC = 0x200;

    // Argument validation
    if(argc < 2) {
        printf("No arguments provided.\n");
        return 1;
    }
    // If user requested help for input we print how arguments should be passed
    else if ((strcmp(argv[1], "-help") == 0) || (strcmp(argv[1], "-h") == 0)) {
        printf("Expected behavior is ./chip_8_emulator arguments path_to_ch8_rom\n");
        printf("arguments are -BGCOLOR = any raylib color, -PCOLOR = any raylib color\n");
        printf("-SPEED=float, -SCALE_FACTOR=int, -debug=bool, -walkthrough=bool \n");
        printf("-vf_reset=bool, -memory_quirk=bool, -display_wait=bool, -clipping_quirk=bool, shifting_quirk=bool, -jumping_quirk=bool\n");
        printf("Available colors are: darkgray, maroon, orange, darkgreen, darkblue, darkpurple, darkbrown, ");
        printf("gray, red, gold, lime, blue, violet, brown, lightgray, pink, yellow, green, skyblue, purple, beige, black, white\n");

        return 0;
    }
    // Debug print out the number and the arguments
    if (debug){
        printf("Number of args: %d\n", argc);
        for (int i = 0; i < argc; i++) {
            printf("Arg %d: %s\n", i, argv[i]);
        }
    }

    // Declare and init background and primary Color with defaults
    Color background;
    Color primary;
    background = WHITE;
    primary = BLACK;

    // Delcare and init scale factor with a default of 10 (modest)
    int scale_factor;
    scale_factor = 10;

    // Declare and init speed scaler with a default of 1x (realtime)
    float speed_scaler;
    speed_scaler = 1.0f;

    // Debug used to hold end location of strings as their proccessed into none string data
    char *endptr;

    /* Process all our arguments as requested */
    for (int i = 1; i < argc; i++) {
        // if a background color is requested set it
        if (strncmp(argv[i], "-BGCOLOR=", 9) == 0) {
            background = get_color_from_name(argv[i] + 9);
            printf("Background color: %s\n", argv[i] + 9);
        }
        // else if a background color is requested set it
        else if (strncmp(argv[i], "-PCOLOR=", 8) == 0) {
            primary = get_color_from_name(argv[i] + 8);
            printf("Primary color: %s\n", argv[i] + 8);
        }
        // else if a speed is requested set it
        else if (strncmp(argv[i], "-SPEED=", 7) == 0) {

            // set our grab in a temp incase its bad input
            float value = strtof(argv[i] + 7, &endptr);

            // if no valid float was given but -SPEED was passed we have bad input so exit
            if (*endptr != '\0') {
                printf("Error: -SPEED must be a valid number.\n");
                return 1;
            }

            // If speed was requested as a negative we have bad input so exit
            if (value <= 0.0f) {
                printf("Error: -SPEED must be positive.\n");
                return 1;
            }

            // Validated data was good so set it and print to the user
            speed_scaler = value;
            printf("Speed set to %.2f\n", speed_scaler);
        }
        // else if scale factor is requested set it
        else if (strncmp(argv[i], "-SCALE_FACTOR=", 14) ==0) {

            // set our grab in a temp incase its bad input
            int value = strtol(argv[i] + 14, &endptr, 10);

            // if no valid int was given but -SCALE_FACTOR was passed we have bad input so exit
            if (*endptr != '\0') {
                printf("Error: -SCALE_FACTOR must be a valid number.\n");
                return 1;
            }

            // If scale factor was requested as a negative we have bad input so exit
            if (value <= 0) {
                printf("Error: -SPEED must be positive.\n");
                return 1;
            }

            // Validated data was good so set it and print to the user
            scale_factor = value;
            printf("Scale factor: %s\n", argv[i] + 14);

        }
        // else if we got a debug we check if true else always false (bad input = false)
        else if (strncmp(argv[i], "-debug=", 7) == 0) {
            bool value = (strncmp(argv[i] + 7, "true", 4) == 0);

            printf("Debug: %s\n", value ? "true" : "false");

            debug = value;
        }
        // else if we got the walkthrough we check if true else always false (bad input = false)
        else if (strncmp(argv[i], "-walkthrough=", 13) == 0) {
            bool value = (strncmp(argv[i] + 13, "true", 4) == 0);

            printf("Walkthrough: %s\n", value ? "true" : "false");

            walk_through_each_instruction = value;

        }
        // else if we got vf_reset quirk (bad input = default)
        else if (strncmp(argv[i], "-vf_reset=", 10) == 0) {
            bool value = (strncmp(argv[i] + 10, "true", 4) == 0);

            bool check_input = (strncmp(argv[i] + 10, "false", 5) == 0);

            if (!value && !check_input) {
                // bad input so we default
            }
            else if (!value && check_input) {
                vf_reset_quirk = false;
            }
            else {
                vf_reset_quirk = true;
            }

            printf("VF_Reset: %s\n", vf_reset_quirk ? "true" : "false");
        }
        // else if we got memory quirk (bad input = default)
        else if (strncmp(argv[i], "-memory_quirk=", 14) == 0) {
            bool value = (strncmp(argv[i] + 14, "true", 4) == 0);

            bool check_input = (strncmp(argv[i] + 14, "false", 5) == 0);

            if (!value && !check_input) {
                // bad input so we default
            }
            else if (!value && check_input) {
                memory_quirk = false;
            }
            else {
                memory_quirk = true;
            }

            printf("Memory quirk: %s\n", memory_quirk ? "true" : "false");
        }
        // else if we got display wait quirk (bad input = default)
        else if (strncmp(argv[i], "-display_wait=", 14) == 0) {
            bool value = (strncmp(argv[i] + 14, "true", 4) == 0);

            bool check_input = (strncmp(argv[i] + 14, "false", 5) == 0);

            if (!value && !check_input) {
                // bad input so we default
            }
            else if (!value && check_input) {
                display_wait_quirk = false;
            }
            else {
                display_wait_quirk = true;
            }

            printf("Display wait quirk: %s\n", display_wait_quirk ? "true" : "false");
        }
        // else if we got clipping quirk (bad input = default)
        else if (strncmp(argv[i], "-clipping_quirk=", 16) == 0) {
            bool value = (strncmp(argv[i] + 16, "true", 4) == 0);

            bool check_input = (strncmp(argv[i] + 16, "false", 5) == 0);

            if (!value && !check_input) {
                // bad input so we default
            }
            else if (!value && check_input) {
                clipping_quirk = false;
            }
            else {
                clipping_quirk = true;
            }

            printf("clipping quirk: %s\n", clipping_quirk ? "true" : "false");
        }
        // else if we got shifting quirk (bad input = default)
        else if (strncmp(argv[i], "-shifting_quirk=", 16) == 0) {
            bool value = (strncmp(argv[i] + 16, "true", 4) == 0);

            bool check_input = (strncmp(argv[i] + 16, "false", 5) == 0);

            if (!value && !check_input) {
                // bad input so we default
            }
            else if (!value && check_input) {
                shifting_quirk = false;
            }
            else {
                shifting_quirk = true;
            }

            printf("shifting quirk: %s\n", shifting_quirk ? "true" : "false");
        }
        // else if we got jumping quirk (bad input = default)
        else if (strncmp(argv[i], "-jumping_quirk=", 15) == 0) {
            bool value = (strncmp(argv[i] + 15, "true", 4) == 0);

            bool check_input = (strncmp(argv[i] + 15, "false", 5) == 0);

            if (!value && !check_input) {
                // bad input so we default
            }
            else if (!value && check_input) {
                jumping_quirk = false;
            }
            else {
                jumping_quirk = true;
            }

            printf("jumping quirk: %s\n", jumping_quirk ? "true" : "false");
        }

    }

    // last argument should always be path to chip 8 rom
    FILE *rom = fopen(argv[argc-1], "rb");
    if (!rom) {
        printf("Failed to open ROM file: %s\n", argv[argc-1]);
        return 1;
    }

    // Read in the rom to memory and set how many bytes were written into this variable
    size_t bytes_read = fread(&chip_8_instance.memory[rom_start_address], 1, memory_size - rom_start_address, rom);
    fclose(rom);

    // Init the window and audio device
    InitWindow(64 * scale_factor, 32 * scale_factor, "CHIP-8 Emulator");
    InitAudioDevice();

    // Load sound file for our chip 8's emulated sound
    Sound beep = LoadSound("beep.wav");

    // Variable used to hold the next read instruction from memory
    unsigned short instruction;
    // Temporary variable used to work on instructions
    u8 temporary_u8;

    // Holds the instructions per second
    int instructions_performed_last_second;

    // Variable used to hold whatever key has been pressed
    u8 current_key_pressed;

    // timespec variable used to hold WHEN we should draw our NEXT frame
    struct timespec when_next_frame;
    // Init it as we need to draw our first frame to get raylib to start a window
    clock_gettime(CLOCK_MONOTONIC, &when_next_frame);

    // Init all input keys as if they were pressed 1000 seconds ago
    for (int i = 0; i < 16; i++){
        // make it “old” so it won’t trigger again until pressed
        chip_8_instance.when_key_last_pressed[i].tv_sec -= 1000; // 1000s ago

    }

    // Variables used to pick indexes on our emulated display
    u8 x_coordinate;
    u8 y_coordinate;

    // Variable used to hold inputs when in debug mode
    char input[100];

    // Variables to control how much time between each instructions given a 660 as realtime with
    // a scaler if we want to run faster than realtime
    float instruction_per_second = speed_scaler * 660.0f;
    float time_per_instruction_ms =  1000.0f / instruction_per_second;

    // float used to adjust instruction timing to be closer to our goal ips
    float adjustment_ratio;
    // Since code execution (emulator), chip 8 execution, and inaccuracy of sleep we give some wiggle room
    time_per_instruction_ms += ((float)time_per_instruction_ms * -.15);

    // Variable used to hold our fonts
    u8 fonts[16][5] = {
        {0xF0, 0x90, 0x90, 0x90, 0xF0}, // 0
        {0x20, 0x60, 0x20, 0x20, 0x70}, // 1
        {0xF0, 0x10, 0xF0, 0x80, 0xF0}, // 2
        {0xF0, 0x10, 0xF0, 0x10, 0xF0}, // 3
        {0x90, 0x90, 0xF0, 0x10, 0x10}, // 4
        {0xF0, 0x80, 0xF0, 0x10, 0xF0}, // 5
        {0xF0, 0x80, 0xF0, 0x90, 0xF0}, // 6
        {0xF0, 0x10, 0x20, 0x40, 0x40}, // 7
        {0xF0, 0x90, 0xF0, 0x90, 0xF0}, // 8
        {0xF0, 0x90, 0xF0, 0x10, 0xF0}, // 9
        {0xF0, 0x90, 0xF0, 0x90, 0x90}, // A
        {0xE0, 0x90, 0xE0, 0x90, 0xE0}, // B
        {0xF0, 0x80, 0x80, 0x80, 0xF0}, // C
        {0xE0, 0x90, 0x90, 0x90, 0xE0}, // D
        {0xF0, 0x80, 0xF0, 0x80, 0xF0}, // E
        {0xF0, 0x80, 0xF0, 0x80, 0x80}  // F
    };
    // Set the fonts to be inside our emulated chip 8s memory starting at FONT_START (0x50)
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 5; j++) {
            chip_8_instance.memory[FONT_START + i * 5 + j] = fonts[i][j];
        }
    }

    /* Set up our graphics */

    // Init the window with a black background
    BeginDrawing();
    ClearBackground(background);
    EndDrawing();

    /* Start emulation loop */

    // While we haven't read all of the ROM
    while(chip_8_instance.PC < (rom_start_address + bytes_read)) {

        /* Fetch instruction */

        // Get the next instruction and increment out PC by 2 (2 byte instruction size)
        instruction = (chip_8_instance.memory[chip_8_instance.PC] << 8) | chip_8_instance.memory[chip_8_instance.PC + 1];
        chip_8_instance.PC += 2;

        // Debug statements
        if (debug){
            printf("On instruction address: 0x%04X, which is: 0x%04X\n", chip_8_instance.PC, instruction);
        }

        // If our time register is not 0 and we aren't currently playing a sound then we do play a sound
        if ((chip_8_instance.sound_register > 0) && (!IsSoundPlaying(beep))){
            PlaySound(beep);
        }
        // If our time register for sound is 0 and or we're playing a sound stop the sound
        else{
            StopSound(beep);
        }

        /* Decode instructions and execute it */

        // Switch case for the instruction we grab the first hex value of the 2 byte instruction
        switch ((instruction & 0xF000) >> 12){
            // Cases for 0x0
            case 0x0:
                // Switch on the last byte of the instruction
                switch(instruction & 0x00FF){
                    // Clear the display
                    case 0xE0:
                        memset(chip_8_instance.display, 0, sizeof(chip_8_instance.display));
                        chip_8_instance.display_has_changed = true;
                        if (debug) {
                            printf("Clear the display\n");
                        }
                        break;

                    // Need to return from subroutine AKA pop stack and make it the PC
                    case 0xEE:
                        chip_8_instance.PC = pop(&chip_8_instance.emulated_stack);
                        if (debug){
                            printf("Returing from subroutine to 0x%04X\n", chip_8_instance.PC);
                        }
                        break;

                }
                break;

            // Jump to address NNN
            case 0x1:
                chip_8_instance.PC = instruction & 0x0FFF;
                if (debug){
                    printf("Jump to address 0x%04X\n", chip_8_instance.PC);
                }
                break;

            // this case we actually do a call of the subroutine
            case 0x2:
                push(&chip_8_instance.emulated_stack, chip_8_instance.PC);
                chip_8_instance.PC = instruction & 0x0FFF;
                if (debug){
                    printf("Call address 0x%04X\n", chip_8_instance.PC);
                }
                break;

            // Skip 1 instruction if the value of Vx is equal to NN
            case 0x3:
                if (chip_8_instance.V[(instruction & 0x0F00) >> 8] == (instruction & 0x00FF)){
                    chip_8_instance.PC += 2;
                }
                if (debug){
                    printf("Checked if 0x%02X is equal to 0x%02X\n", chip_8_instance.V[(instruction & 0x0F00) >> 8], instruction & 0x00FF);
                }
                break;

            // Skip 1 instruction if the value Vx is not equal to NN
            case 0x4:
                if (chip_8_instance.V[(instruction & 0x0F00) >> 8] != (instruction & 0x00FF)){
                    chip_8_instance.PC += 2;
                }
                if (debug){
                    printf("Checked if 0x%02X is not equal to 0x%02X\n", chip_8_instance.V[(instruction & 0x0F00) >> 8], instruction & 0x00FF);
                }
                break;

            // Skip 1 instruction if Vx and Vy are equal
            case 0x5:
                if (chip_8_instance.V[(instruction & 0x0F00) >> 8] == chip_8_instance.V[(instruction & 0x00F0) >> 4]){
                    chip_8_instance.PC += 2;
                }
                if (debug){
                    printf("Checked if V[%d] = 0x%02X is equal to V[%d] = 0x%02X\n", ((instruction & 0x0F00) >> 8), chip_8_instance.V[(instruction & 0x0F00) >> 8], ((instruction & 0x00F0) >> 4), chip_8_instance.V[(instruction & 0x00F0) >> 4]);
                }
                break;

            // Set Vx to NN
            case 0x6:
                chip_8_instance.V[(instruction & 0x0F00) >> 8] = instruction & 0x00FF;
                if (debug){
                    printf("Set V[%d] to 0x%02X\n", (instruction & 0x0F00) >> 8, instruction & 0x00FF);
                }
                break;

            // Add NN to Vx (without carry) and without changing the carry flag
            case 0x7:
                chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x0F00) >> 8] + (instruction & 0x00FF);
                if (debug){
                    printf("Add 0x%02X to V[%d], result: 0x%02X\n", instruction & 0x00FF, (instruction & 0x0F00) >> 8, chip_8_instance.V[(instruction & 0x0F00) >> 8]);
                }
                break;

            // 0x8 is used for a lot of logical and arthmetic instructions so we switch for each
            case 0x8:
                switch (instruction & 0x000F){
                    // Binary Set operation
                    case 0x0:
                        chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x00F0) >> 4];
                        if (debug){
                            printf("Set V[%d] to equal V[%d]\n", ((instruction & 0x0F00) >> 8), ((instruction & 0x00F0) >> 4));
                        }
                        break;

                    // Binary Or operation
                    case 0x1:
                        chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x0F00) >> 8] | chip_8_instance.V[(instruction & 0x00F0) >> 4];
                        if (debug){
                            printf("Set V[%d] to the or operation of V[%d] | V[%d]\n", ((instruction & 0x0F00) >> 8), ((instruction & 0x0F00) >> 8), ((instruction & 0x00F0) >> 4));
                        }
                        if (vf_reset_quirk){
                            chip_8_instance.V[15] = 0;
                            if (debug) {
                                printf("Reset flag register V[15]\n");
                            }
                        }
                        break;

                    // Binary And operation
                    case 0x2:
                        chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x0F00) >> 8] & chip_8_instance.V[(instruction & 0x00F0) >> 4];
                        if (debug){
                            printf("Set V[%d] to the and operation of V[%d] & V[%d]\n", ((instruction & 0x0F00) >> 8), ((instruction & 0x0F00) >> 8), ((instruction & 0x00F0) >> 4));
                        }
                        if (vf_reset_quirk){
                            chip_8_instance.V[15] = 0;
                            if (debug) {
                                printf("Reset flag register V[15]\n");
                            }
                        }
                        break;

                    // Binary XOR operation
                    case 0x3:
                        chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x0F00) >> 8] ^ chip_8_instance.V[(instruction & 0x00F0) >> 4];
                        if (debug){
                            printf("Set V[%d] to the XOR operation of V[%d] ^ V[%d]\n", ((instruction & 0x0F00) >> 8), ((instruction & 0x0F00) >> 8), ((instruction & 0x00F0) >> 4));
                        }
                        if (vf_reset_quirk){
                            chip_8_instance.V[15] = 0;
                            if (debug) {
                                printf("Reset flag register V[15]\n");
                            }
                        }
                        break;

                    // Binary addition of registers V[x] and V[y] with overflow setting VF to 1 ELSE its set to 0
                    case 0x4:
                        if ((chip_8_instance.V[(instruction & 0x0F00) >> 8] + chip_8_instance.V[(instruction & 0x00F0) >> 4]) > 255){
                            chip_8_instance.V[(instruction & 0x0F00) >> 8] = (chip_8_instance.V[(instruction & 0x0F00) >> 8] + chip_8_instance.V[(instruction & 0x00F0) >> 4]);
                            chip_8_instance.V[15] = 1;
                        }
                        else {
                            chip_8_instance.V[(instruction & 0x0F00) >> 8] = (chip_8_instance.V[(instruction & 0x0F00) >> 8] + chip_8_instance.V[(instruction & 0x00F0) >> 4]);
                            chip_8_instance.V[15] = 0;
                        }
                        if (debug){
                            printf("Added V[%d] with V[%d] placed in V[%d] did overflow: %s\n", (instruction & 0x0F00) >> 8, (instruction & 0x00F0) >> 4, (instruction & 0x0F00) >> 8, (chip_8_instance.V[15] == 1) ? "True" : "False");
                        }
                        break;

                    // 8XY5 - VX = VX - VY, VF = NOT borrow
                    case 0x5:
                        if (chip_8_instance.V[(instruction & 0x0F00) >> 8] >= chip_8_instance.V[(instruction & 0x00F0) >> 4]){
                            temporary_u8 = 1;
                        }
                        else{
                            temporary_u8 = 0;
                        }
                        chip_8_instance.V[(instruction & 0x0F00) >> 8] -= chip_8_instance.V[(instruction & 0x00F0) >> 4];
                        chip_8_instance.V[15] = temporary_u8;
                        if (debug){
                            printf("Substracted V[%d] from V[%d] placed in V[%d] carry flag: %s\n", (instruction & 0x00F0) >> 4, (instruction & 0x0F00) >> 8, (instruction & 0x0F00) >> 8, (chip_8_instance.V[15] == 1) ? "1" : "0");
                        }
                        break;

                    // 8XY7 - VX = VY - VX, VF = NOT borrow
                    case 0x7:
                        if (chip_8_instance.V[(instruction & 0x00F0) >> 4] >= chip_8_instance.V[(instruction & 0x0F00) >> 8]){
                            temporary_u8 = 1;
                        }
                        else{
                            temporary_u8 = 0;
                        }
                        chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x00F0) >> 4] - chip_8_instance.V[(instruction & 0x0F00) >> 8];
                        chip_8_instance.V[15] = temporary_u8;
                        if (debug){
                            printf("Substracted V[%d] from V[%d] placed in V[%d] carry flag: %s\n", (instruction & 0x0F00) >> 8, (instruction & 0x00F0) >> 4, (instruction & 0x0F00) >> 8, (chip_8_instance.V[15] == 1) ? "1" : "0");
                        }
                        break;

                    // Shift V[X] by 1 bit (right) if the bit shifted out was 1 set V[F] to 1 else set it to 0
                    case 0x6:
                        if (chip_8_instance.V[(instruction & 0x0F00) >> 8] & 0b00000001){
                            temporary_u8 = 1;

                        }
                        else{
                            temporary_u8 = 0;
                        }
                        if (shifting_quirk) {
                            chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x0F00) >> 8] >> 1;
                            chip_8_instance.V[15] = temporary_u8;
                        }
                        else {
                            chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x00F0) >> 4] >> 1;
                            chip_8_instance.V[15] = chip_8_instance.V[(instruction & 0X00F0) >> 4] & 0b00000001;
                        }

                        if (debug){
                            printf("Shifted V[%d] by 1 bit to the right, shifted out %d into V[F]\n", (instruction & 0x0F00) >> 8, chip_8_instance.V[15]);
                        }
                        break;

                    // Shift V[X] by 1 bit (left) if the bit shifted out was 1 set V[F] to 1 else set it to 0
                    case 0xE:
                        if (chip_8_instance.V[(instruction & 0x0F00) >> 8] & 0b10000000){
                            temporary_u8 = 1;

                        }
                        else{
                            temporary_u8 = 0;
                        }
                        if (shifting_quirk) {
                            chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x0F00) >> 8] << 1;
                            chip_8_instance.V[15] = temporary_u8;
                        }
                        else {
                            chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x00F0) >> 4] << 1;
                            chip_8_instance.V[15] = (chip_8_instance.V[(instruction & 0X00F0) >> 4] & 0b10000000) >> 7;
                        }

                        if (debug){
                            printf("Shifted V[%d] by 1 bit to the left, shifted out %d into V[F]\n", (instruction & 0x0F00) >> 8, chip_8_instance.V[15]);
                        }
                        break;

                }
                break;

            // Skip 1 instruction if Vx and Vy are not equal
            case 0x9:
                if (chip_8_instance.V[(instruction & 0x0F00) >> 8] != chip_8_instance.V[(instruction & 0x00F0) >> 4]){
                    chip_8_instance.PC += 2;
                }
                if (debug){
                    printf("If V[%d] = 0x%02X is not equal to V[%d] = 0x%02X skip an instruction\n", ((instruction & 0x0F00) >> 8), chip_8_instance.V[(instruction & 0x0F00) >> 8], ((instruction & 0x00F0) >> 4), chip_8_instance.V[(instruction & 0x00F0) >> 4]);
                }
                break;

            // Set the index register to NNN
            case 0xA:
                chip_8_instance.I = (instruction & 0x0FFF);
                if (debug){
                    printf("Set I to 0x%03X\n", chip_8_instance.I);
                }

                break;

            // Jump with an offset (original implementation)
            case 0xB:
                if (jumping_quirk) {
                    chip_8_instance.PC = (instruction & 0x0FFF) + chip_8_instance.V[(instruction & 0X0F00) >> 8];
                }
                else {
                    chip_8_instance.PC = (instruction & 0x0FFF) + chip_8_instance.V[0];
                }
               if (debug){
                   printf("Jump to %d\n", chip_8_instance.PC);
               }
               break;

            // Get a random value (0 - 255 inclusive) then binary and it with the two last nibbles of the instruction
            case 0xC:
               chip_8_instance.V[(instruction & 0x0F00) >> 8] = GetRandomValue(0, 255) & (instruction & 0x00FF);
               if (debug){
                   printf("Got a random int: %d\n", (GetRandomValue(0, 255) & (instruction & 0x00FF)));
               }
               break;

            // Case of us writting a sprite to the screen
            case 0xD:
                if (chip_8_instance.display_wait_timer == 0){

                    // DXYN
                    // set the X coordinate to the value in VX (V register N number) modulo 64
                    x_coordinate = chip_8_instance.V[(instruction & 0x0F00) >> 8] & 63;
                    y_coordinate = chip_8_instance.V[(instruction & 0x00F0) >> 4] & 31;
                    chip_8_instance.V[15] = 0;


                    for (int i = 0; i < (instruction & 0x000F); i++) {
                        u8 sprite_data = chip_8_instance.memory[chip_8_instance.I + i];
                        // wrap
                        int y = (y_coordinate + i) % chip_8_screen_height;
                        // if we're not supposed to wrap break if we would
                        if (clipping_quirk && (y_coordinate + i) > chip_8_screen_height) {
                            break;
                        }

                        for (int j = 0; j < 8; j++) {
                            // wrap
                            int x = (x_coordinate + j) % chip_8_screen_width;
                            // if we're not supposed to wrap break if we would
                            if (clipping_quirk && (x_coordinate + j) > chip_8_screen_width) {
                                break;
                            }
                            if (sprite_data & (0x80 >> j)) {
                                int display_index = y * chip_8_screen_width + x;
                                if (chip_8_instance.display[display_index]) {
                                    chip_8_instance.display[display_index] = 0;
                                    chip_8_instance.V[15] = 1;
                                } else {
                                    chip_8_instance.display[display_index] = 1;
                                }
                            }
                        }
                    }
                    chip_8_instance.display_has_changed = true;

                    if (display_wait_quirk) {
                        chip_8_instance.display_wait_timer += 1;
                    }

                    if (debug){
                        printf("Wrote %d tall sprite at X = %d and Y = %d\n", (instruction & 0X00F), (instruction & 0x0F00) >> 8, (instruction & 0x00F0) >> 4);
                    }
                }
                else {
                    chip_8_instance.PC -= 2;
                }
                break;

            // Cases for input (that aren't blocking)
            case 0xE:
                // Switch on the last byte
                switch (instruction & 0x00FF) {
                    // Skip if key in Vx is pressed
                    case 0x9E: {
                        PollInputEvents();
                        temporary_u8 = chip_8_instance.V[(instruction & 0x0F00) >> 8];
                        current_key_pressed = get_most_recent_input(&chip_8_instance);

                        if (temporary_u8 == current_key_pressed) {  // check the correct key
                            chip_8_instance.PC += 2;
                        }

                        if (debug){
                            printf("Skip if 0x%01X == 0x%01X\n", temporary_u8, current_key_pressed);
                        }

                        break;
                    }
                    // Skip if key in Vx is NOT pressed
                    case 0xA1: {
                        PollInputEvents();
                        temporary_u8 = chip_8_instance.V[(instruction & 0x0F00) >> 8];
                        current_key_pressed = get_most_recent_input(&chip_8_instance);

                        if (!((temporary_u8 == current_key_pressed) && (temporary_u8 != 0xFF))) {
                            chip_8_instance.PC += 2;
                        }

                        if (debug){
                            printf("Skip if 0x%01X != 0x%01X\n", temporary_u8, current_key_pressed);
                        }

                        break;
                    }
                }

               break;


            // Opcodes for timers, setting, and reading
            case 0xF:
                // switch on the last byte
                switch (instruction & 0x00FF){
                    // Set V[X] to the current value of our delay_register (timer)
                    case 0x07:
                        chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.delay_register;
                        if (debug){
                            printf("Set %d to the delay_registers value of %d\n", (instruction & 0x0F00) >> 8, chip_8_instance.delay_register);
                        }
                        break;
                    // Set the delay_register (timer) to V[X]
                    case 0x15:
                        chip_8_instance.delay_register = chip_8_instance.V[(instruction & 0x0F00) >> 8];
                        if (debug){
                            printf("Set delay register to V[%d] = %d\n", (instruction & 0x0F00) >> 8, chip_8_instance.V[(instruction & 0x0F00) >> 8]);
                        }
                        break;
                    // Set the sound_register (timer for sound) to V[X]
                    case 0x18:
                        chip_8_instance.sound_register = chip_8_instance.V[(instruction & 0x0F00) >> 8];
                        if (debug){
                            printf("Set sound register to V[%d] = %d\n", (instruction & 0x0F00) >> 8, chip_8_instance.V[(instruction & 0x0F00) >> 8]);
                        }
                        break;
                    // Add V[X] to our index register (ambiguous behavior)
                    case 0x1E:
                        chip_8_instance.I += chip_8_instance.V[(instruction & 0x0F00) >> 8];
                        if (debug){
                            printf("Add V[%d] to index register\n", (instruction & 0x0F00) >> 8);
                        }
                        break;
                    // Get a key blocking until a key is recieved
                    case 0x0A:
                        PollInputEvents();
                        temporary_u8 = get_most_recent_input(&chip_8_instance);
                        if (temporary_u8 != 0xFF){
                            chip_8_instance.V[(instruction & 0xF00) >> 8] = temporary_u8;
                            if (debug){
                                printf("Got input: 0x%01X\n", temporary_u8);
                            }
                        }
                        else{
                            chip_8_instance.PC -= 2;
                            if (debug){
                                printf("Waiting for input\n");
                            }
                        }
                        break;
                    // Set our index register to the requested font V[X]
                    case 0x29:
                        chip_8_instance.I = 0x50 + (chip_8_instance.V[(instruction & 0x0F00) >> 8] * 5);
                        if (debug){
                            printf("Setting index register to font: 0x%01X\n", chip_8_instance.V[(instruction & 0x0F00) >> 8]);
                        }
                        break;
                    // Binary-Coded decimal conversion i = (V[X] / 100), i + 1 = (V[X] % 100) / 10, i + 2 = (V[X] % 10)
                    // AKA we take each digit of of V[X] and place them individually in I incrementing for each digit
                    case 0x33:
                        temporary_u8 = chip_8_instance.V[(instruction & 0x0F00) >> 8];
                        chip_8_instance.memory[chip_8_instance.I] = temporary_u8 / 100;
                        chip_8_instance.memory[chip_8_instance.I + 1] = (temporary_u8 % 100) / 10;
                        chip_8_instance.memory[chip_8_instance.I + 2] = (temporary_u8 % 10);

                        if (debug){
                            printf("BCD - Start\n");
                            printf("Memory address[%d] = (V[%d] / 100) = %d\n", chip_8_instance.I, (instruction & 0x0F00) >> 8, chip_8_instance.memory[chip_8_instance.I]);
                            printf("Memory address[%d] = (V[%d] %% 100) / 10 = %d\n", chip_8_instance.I + 1, (instruction & 0x0F00) >> 8, chip_8_instance.memory[chip_8_instance.I + 1]);
                            printf("Memmory address[%d] = (V[%d] %% 10) = %d\n", chip_8_instance.I + 2, (instruction & 0x0F00) >> 8, chip_8_instance.memory[chip_8_instance.I + 2]);
                            printf("BCD - End\n");
                        }

                        break;
                    // We change our emulated memory to the registers from 0 to X
                    // AKA overwrite our emulated memory starting at i with V[0] till i + x = V[X]
                    case 0x55:
                        temporary_u8 = ((instruction & 0x0F00) >> 8);
                        for( u8 i = 0; i <= temporary_u8; i++){
                            chip_8_instance.memory[chip_8_instance.I + i] = chip_8_instance.V[i];
                            if (debug) {
                                printf("Overwriting memory address[%d] with %d\n", chip_8_instance.I + i,  chip_8_instance.V[i]);
                            }
                        }
                        if (memory_quirk){
                            chip_8_instance.I += 1;
                        }
                        break;
                    // We change our emulated registers to the memory from 0 to X
                    // AKA Overwrite our registers starting at V[0] with memory[i] till V[X] = memory[i] + x
                    case 0x65:
                        temporary_u8 = ((instruction & 0x0F00) >> 8);
                        for( u8 i = 0; i <= temporary_u8; i++){
                            chip_8_instance.V[i] = chip_8_instance.memory[chip_8_instance.I + i];
                            if (debug) {
                                printf("Overwriting V[%x] with memory address[%d]\n", i, chip_8_instance.I);
                            }
                        }
                        if (memory_quirk){
                            chip_8_instance.I += 1;
                        }
                        break;
                }
                break;

            // Default case for unimplemented instructions / bad data
            default:
                printf("Unknown instruction\n");
                break;
        }

        // Debug controller that takes input for each instruction to allow instruction by instruction debugging
        if (walk_through_each_instruction){
            printf("Enter a command: ");
            fgets(input, sizeof(input), stdin);

            // Remove newline if present
            input[strcspn(input, "\n")] = 0;

            // If the user just hit enter
            if (input[0] == '\0'){

            }
            // The user gave some command
            else{
                if (strcmp(input, "print") == 0){
                    print_chip_8_contents(&chip_8_instance);

                }
            }

         }

         // Update the time registers (decrement if its been 1/60 of a second)
         update_time_registers(&chip_8_instance);

         /* Draw our frame if its time */

         // If it's time to print a frame print the frame and reset our instruction count
         if (time_has_passed(&when_next_frame)){
            // Prep buffer for editing
            BeginDrawing();
            draw_frame(&chip_8_instance, scale_factor, &primary, &background);
            // Draw the edited buffer to the screen
            EndDrawing();
            // Update for when we should print another frame
            when_next_frame = make_future_time(16.6667);
         }

         /* Get inputs from the user */

         // Get what keys are pressed and set out timespec array for each character with when/if it was pressed
         PollInputEvents();
         if (IsKeyDown(KEY_ONE))    clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0x1]);
         if (IsKeyDown(KEY_TWO))    clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0x2]);
         if (IsKeyDown(KEY_THREE))  clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0x3]);
         if (IsKeyDown(KEY_FOUR))   clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0xC]);

         if (IsKeyDown(KEY_Q))      clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0x4]);
         if (IsKeyDown(KEY_W))      clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0x5]);
         if (IsKeyDown(KEY_E))      clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0x6]);
         if (IsKeyDown(KEY_R))      clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0xD]);

         if (IsKeyDown(KEY_A))      clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0x7]);
         if (IsKeyDown(KEY_S))      clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0x8]);
         if (IsKeyDown(KEY_D))      clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0x9]);
         if (IsKeyDown(KEY_F))      clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0xE]);

         if (IsKeyDown(KEY_Z))      clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0xA]);
         if (IsKeyDown(KEY_X))      clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0x0]);
         if (IsKeyDown(KEY_C))      clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0xB]);
         if (IsKeyDown(KEY_V))      clock_gettime(CLOCK_MONOTONIC, &chip_8_instance.when_key_last_pressed[0xF]);

         /* Keep track of instruction speed and adjust as necessary */

         // track this instruction (if its been a second we get a print out of how many instructions we did in that second)
         instructions_performed_last_second = track_instructions();

         // Adjust how long we sleep in accordance with how many instructions we want to do vs what we actually did
         if (instructions_performed_last_second > 0 && !debug) {
             adjustment_ratio = instruction_per_second / (float)instructions_performed_last_second;

             // If debug is on we print our target instructions per second as value and ratio
             if (debug){
                 printf("Target instructions per second: %f\n", instruction_per_second);
                 printf("Ratio: %f\n", adjustment_ratio);
             }

             // Get how far we are off as a value
             adjustment_ratio -= 1.0;
             // Adjust how long we should sleep to we're closer to our target instructions per second
             time_per_instruction_ms += (time_per_instruction_ms * (adjustment_ratio * -1));

         }

         /* Sleep till next instruction using delta time */

         // Sleep with delta time
         sleep_for_instruction(time_per_instruction_ms);

    }

    // Clean up
    // unload our beep audio file
    UnloadSound(beep);
    // Close the audio device
    CloseAudioDevice();
    // Close the window
    CloseWindow();
    return 0;

}
