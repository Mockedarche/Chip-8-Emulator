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
#include "chip_8_emulator.h"
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>

// Width of the CHIP-8 display
const int chip_8_screen_width = 64;

// Height of the CHIP-8 display
const int chip_8_screen_height = 32;

// Scale factor for enlarging the CHIP-8 display (for modern displays)
const int scale_factor = 25;

// Width of the emulated window
const int emulated_screen_width = chip_8_screen_width * scale_factor;

// Height of the emulated window
const int emulated_screen_height = chip_8_screen_height * scale_factor;

// CHIP-8 has 4096 bytes of memory
const int memory_size = 4096;

// Starting memory address for fonts
#define FONT_START 0x50

// Programs start at memory location 0x200 (512)
const int rom_start_address = 0x200;

// DEBUG FLAG
const bool debug = false;

// Flag for quirks (different emulators act slightly different this flag changes those quirks as desired)
bool old_implementation = false;


// Converts current time to its ms representation as a long
unsigned long current_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000UL) + (tv.tv_usec / 1000UL);
}


/*
 * Custom Stack object
 * Expects top to be set to -1 on initialization
 *
 */
typedef struct stack {
    u16 emulated_stack_array[16];
    int top;


} stack;

/*
 * push function
 * Expects: The emulated stack to be of size 16 and ONLY 16
 * Does: Pushes the u16 variable to the emulated stack
 *
 */
void push (stack *emulated_stack, u16 to_push){
    if (emulated_stack->top >= 15){
        printf("ERROR ERROR program attempted to push to stack when stack FULL\n");
        return;
    }
    else {
        emulated_stack->top += 1;
        emulated_stack->emulated_stack_array[emulated_stack->top] = to_push;
    }


}

/*
 * pop function
 * Expects: Emulated stack to be initialized with a size > 0
 * Does: Pops the top of the stack and returns the u16 value
 *
 */
u16 pop (stack *emulated_stack){
    if (emulated_stack->top < 0){
        printf("ERROR ERROR program attempted to pop from an EMPTY stack\n");
        return 0;
    }
    else {
        u16 temp = emulated_stack->emulated_stack_array[emulated_stack->top];
        emulated_stack->top -= 1;
        return temp;
    }
}


/*
 * chip_8 struct
 * Expects: N/A
 * Does: Defines the structure of the CHIP-8 emulator, including memory, registers, stack, and display
 */
typedef struct chip_8 {

    // The Chip 8's memory
    u8 memory[memory_size];

    // The Chip 8's registers
    u8 V[16];

    // The chip 8's stack using an emulated one for ease of use
    stack emulated_stack;

    // Special registers
    u16 I;
    // Program Counter
    u16 PC;
    // Stack Pointer
    u8 SP;
    // Delay timer
    u8 delay_register;
    // Sound timer
    u8 sound_register;

    //SPECIAL VALUE USED FOR TIME
    unsigned long last_update;

    // The Chip 8's display (64 x 32 pixels)
    b8 display[64 * 32];

    // boolean to denote if something changed in the display_array
    bool display_has_changed;

    struct timespec when_key_last_pressed[16];


} chip_8;

/*
 * update_time_registers function
 * Exxpects: chip 8 object to be correctly initialized
 * Does: If a second has passed we decrement any timers above 0 by 1
 *
 */
void update_time_registers(chip_8 *chip_8_object){
   unsigned long now = current_ms();
   if ((now - chip_8_object->last_update) > (1000.0f / 60.0f)){
      if (chip_8_object->delay_register > 0){
         chip_8_object->delay_register -= 1;
      }
      if (chip_8_object->sound_register > 0) {
         chip_8_object->sound_register -= 1;
      }

      chip_8_object->last_update = now;
   }
}


/*
 * draw_frame function
 * Expects: a pointer to a chip_8 struct
 * Does: Draws the current state of the CHIP-8 display to the raylib window
 *
 */
int draw_frame(chip_8 *chip_8_object){


    for (int i = 0; i < chip_8_screen_width; i++){
        for (int j = 0; j < chip_8_screen_height; j++){
            if (chip_8_object->display[j * chip_8_screen_width + i]){
                DrawRectangle(i * scale_factor, j * scale_factor, scale_factor, scale_factor, GREEN);
            }
            else {
                DrawRectangle(i * scale_factor, j * scale_factor, scale_factor, scale_factor, BLACK);
            }
        }
    }

    chip_8_object->display_has_changed = false;
    return 0;
}

/*
 * print_chip_8_contents function
 * Expects: chip 8 object to be initialized correctly
 * Does: Prints out all of the chip 8's emulated hardwares contents (except memory)
 *
 */
int print_chip_8_contents(chip_8 *chip_8_instance){
    printf("Printing the contents of the chip 8 instance\n");
    printf("Chip 8 Registers\n");
    for (int i = 0; i < 16; i++){
        printf("V[%d] = 0x%02X\n", i, chip_8_instance->V[i]);
    }
    printf("\n");
    printf("Chip 8 Stack\n");
    for (int i = 0; i < 16; i++){
        printf("Stack[%d] = 0x%04X\n", i, chip_8_instance->emulated_stack.emulated_stack_array[i]);
    }
    printf("\n");
    printf("Chip 8 Index Register\n");
    printf("I = 0x%04X\n", chip_8_instance->I);
    printf("Chip 8 Program Counter\n");
    printf("PC = 0x%04X\n", chip_8_instance->PC);
    printf("Chip 8 Stack Pointer\n");
    printf("SP = 0x%02X\n", chip_8_instance->SP);

    return 0;
}

/*
 * millis_since helper function - get_most_recent_input
 * Expects: the start timespec struct to be correctly initialized with a time
 * Does: returns how many milliseconds (int) have passed since the given timespec struct
 *
 */
int millis_since(struct timespec start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long diff_ms = (now.tv_sec - start.tv_sec) * 1000;
    diff_ms += (now.tv_nsec - start.tv_nsec) / 1000000;
    return (int)diff_ms;
}

/*
 * get_most_recent_input function
 * Expects: chip_8_object to be correctly initialized
 * Does: Returns which character has been pressed most recently (cap 200 ms old) or 0xFF as none were pressed
 *
 */
u8 get_most_recent_input(chip_8 *chip_8_object) {
    int most_recent_age = 200; // max age in ms
    u8 recent_key = 0xFF;        // default if no key is recent
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
 * pretty_timer function
 * Expects: NA
 * Does: Prints how long in ms it's been since this method was LAST called (using a static variable)
 *
 */
void pretty_timer() {
    static struct timespec last_call = {0, 0};
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (last_call.tv_sec == 0 && last_call.tv_nsec == 0) {
        printf("First call, starting timer...\n");
    } else {
        double elapsed_ms = (now.tv_sec - last_call.tv_sec) * 1000.0 +
                            (now.tv_nsec - last_call.tv_nsec) / 1000000.0;
        printf("Time since last call: %.3f ms\n", elapsed_ms);
    }

    last_call = now;
}

/*
 * sleep_for_instruction function
 * Expects: time_per_instruction_ms to be a valid float
 * Does: Accounts for the entire time since it was last called (minus time it slept) and sleeps for a delta
 * time that accounts for variance
 */
void sleep_for_instruction(float time_per_instruction_ms) {
    static struct timespec last_time = {0, 0};
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (last_time.tv_sec == 0 && last_time.tv_nsec == 0) {
        last_time = now;  // first call
    }

    long elapsed_ns = (now.tv_sec - last_time.tv_sec) * 1000000000L
                    + (now.tv_nsec - last_time.tv_nsec);

    long target_ns = (long)(time_per_instruction_ms * 1000000L);
    long sleep_ns = target_ns - elapsed_ns;

    if (sleep_ns > 0) {
        struct timespec ts;
        ts.tv_sec = sleep_ns / 1000000000L;
        ts.tv_nsec = sleep_ns % 1000000000L;
        nanosleep(&ts, NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &last_time);
}

/*
 * make_future_time function
 * Expects: ms to be a valid double
 * Does: Returns a struct timespec that has its time offset by the provided ms (milliseconds)
 */
struct timespec make_future_time(double ms) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);

    long ns_to_add = (long)(ms * 1000000.0);
    t.tv_nsec += ns_to_add;
    if (t.tv_nsec >= 1000000000) {
        t.tv_sec += t.tv_nsec / 1000000000;
        t.tv_nsec %= 1000000000;
    }
    return t;
}

/*
 * time_has_passed function
 * Expects: *time should be correctly initialized
 * Does: Returns true if targets time has passed else returns false
 *
 */
bool time_has_passed(struct timespec *target) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (now.tv_sec > target->tv_sec) return true;
    if (now.tv_sec == target->tv_sec && now.tv_nsec >= target->tv_nsec) return true;
    return false;
}

/*
 * track_instruction function
 * Expects: NA
 * Does: Counts how many times its called per second and prints that out every second
 * as instructions per second
 *
 */
void track_instructions() {
    static int instruction_count = 0;
    static struct timespec last_time = {0};

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (last_time.tv_sec == 0) last_time = now; // init first call

    instruction_count++;

    double elapsed = (now.tv_sec - last_time.tv_sec) +
                     (now.tv_nsec - last_time.tv_nsec) / 1e9;

    if (elapsed >= 1.0) {
        printf("Instructions per second: %d\n", instruction_count);
        instruction_count = 0;
        last_time = now;
    }
}



/*
 * main function
 * Expects: one argument: path to the CHIP-8 ROM file
 * Does: Initializes the CHIP-8 emulator, loads the ROM into memory, and starts the emulation loop.
 */
int main(int argc, const char * argv[]) {

    // Init the window and audio device
    InitWindow(emulated_screen_width, emulated_screen_height, "CHIP-8 Emulator");
    InitAudioDevice();

    // Load sound file for our chip 8's emulated sound
    Sound beep = LoadSound("beep.wav");

    // Declare and init our chip_8 struct (blanking it to 0 to prevent bad data)
    chip_8 chip_8_instance;
    memset(&chip_8_instance, 0, sizeof(chip_8_instance));

    // Our Emulated stack requires top be init to -1
    chip_8_instance.emulated_stack.top = -1;
    // Last update needs to be assigned a value at start up
    chip_8_instance.last_update = current_ms();
    // Point out program counter to where the rom starts
    chip_8_instance.PC = 0x200;
    // Variable used to hold the next read instruction from memory
    unsigned short instruction;
    // Temporary variable used to work on instructions
    u8 temporary_u8;
    // Variable used to count ran instructions ()
    int instruction_count;
    // Init our instruction count to 0
    instruction_count = 0;
    // Variable used to count how many frames have been drawn
    int frame_count;
    // Init our frame count to 0
    frame_count = 0;

    // Variable used to hold whatever key has been pressed
    u8 current_key_pressed;
    // Temp variable used in handling key inputs
    u8 temp_key_hold;

    // timespec variable used to hold WHEN we should draw our NEXT frame
    struct timespec when_next_frame;
    // Init it as we need to draw our first frame to get raylib to start a window
    clock_gettime(CLOCK_MONOTONIC, &when_next_frame);

    // Init all input keys as if they were pressed 1000 seconds ago
    for (int i = 0; i < 16; i++){
        // make it “old” so it won’t trigger again until pressed again
        chip_8_instance.when_key_last_pressed[i].tv_sec -= 1000; // 1000s ago

    }

    // Variables used to pick indexes on our emulated display
    u8 x_coordinate;
    u8 y_coordinate;

    // Variable used to hold inputs when in debug mode
    char input[100];

    // Variables to control how much time between each instructions given a 660 as realtime with
    // a scaler if we want to run faster than realtime
    float speed_scaler = 1.0f;
    float instruction_per_second = speed_scaler * 660.0f;
    float time_per_instruction_ms =  1000.0f / instruction_per_second;
    long time_to_sleep = (long)(time_per_instruction_ms * 1000000L);

    // timespect variable used for when sleeping
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = (long)(time_per_instruction_ms * 1000000L);


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


    // Debug for arguments provided
    if(argc < 2) {
        printf("No arguments provided.\n");
        return 1;
    }
    else if ((strcmp(argv[1], "-help") == 0) || (strcmp(argv[1], "-h") == 0)) {
        printf("Expected behavior is ./chip_8_emulator path_to_ch8_rom\n");
        return 0;
    }
    printf("Number of args: %d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("Arg %d: %s\n", i, argv[i]);
    }

    // Open the provided rom file
    FILE *rom = fopen(argv[1], "rb");
    if (rom == NULL) {
        printf("Failed to open ROM file: %s\n", argv[1]);
        return 1;
    }

    // Read in the rom to memory and set how many bytes were written into this variable
    size_t bytes_read = fread(&chip_8_instance.memory[rom_start_address], 1, memory_size - rom_start_address, rom);
    fclose(rom);

    // Init the window with a black background
    BeginDrawing();
    ClearBackground(BLACK);
    EndDrawing();

    // While we haven't read all of the ROM
    while(chip_8_instance.PC < (rom_start_address + bytes_read)) {

        // Get the next instruction and increment out PC by 2 (2 byte instruction size)
        instruction = (chip_8_instance.memory[chip_8_instance.PC] << 8) | chip_8_instance.memory[chip_8_instance.PC + 1];
        chip_8_instance.PC += 2;

        // Debug statements
        //printf("On instruction: 0x%04X\n", chip_8_instance.PC);
        //printf("0x%04X\n", instruction);

        // If our time register is not 0 and we aren't currently playing a sound then we do play a sound
        if ((chip_8_instance.sound_register > 0) && (!IsSoundPlaying(beep))){
            PlaySound(beep);
        }
        // If our time register for sound is 0 and or we're playing a sound stop the sound
        else{
            StopSound(beep);
        }

        // Switch case for the instruction we grab the first hex value of the 2 byte instruction
        switch ((instruction & 0xF000) >> 12){
            // Cases for 0x0
            case 0x0:
                // Switch on the last byte of the instruction
                switch(instruction & 0x00FF){
                    // Clear the display
                    case 0xE0:
                        memset(chip_8_instance.display, 0, sizeof(chip_8_instance.display));
                        //printf("Clear the display\n");
                        break;

                    // Need to return from subroutine AKA pop stack and make it the PC
                    case 0xEE:
                        chip_8_instance.PC = pop(&chip_8_instance.emulated_stack);
                        //printf("Returing from subroutine to 0x%04X\n", chip_8_instance.PC);
                        break;

                }
                break;

            // Jump to address NNN
            case 0x1:
                chip_8_instance.PC = instruction & 0x0FFF;
                //printf("Jump to address 0x%04X\n", chip_8_instance.PC);
                break;

            // this case we actually do a call of the subroutine
            case 0x2:
                push(&chip_8_instance.emulated_stack, chip_8_instance.PC);
                chip_8_instance.PC = instruction & 0x0FFF;
                //printf("Call address 0x%04X\n", chip_8_instance.PC);
                break;

            // Skip 1 instruction if the value of Vx is equal to NN
            case 0x3:
                if (chip_8_instance.V[(instruction & 0x0F00) >> 8] == (instruction & 0x00FF)){
                    chip_8_instance.PC += 2;
                }
                //printf("Checked if 0x%02X is equal to 0x%02X\n", chip_8_instance.V[(instruction & 0x0F00) >> 8], instruction & 0x00FF);
                break;

            // Skip 1 instruction if the value Vx is not equal to NN
            case 0x4:
                if (chip_8_instance.V[(instruction & 0x0F00) >> 8] != (instruction & 0x00FF)){
                    chip_8_instance.PC += 2;
                }
                //printf("Checked if 0x%02X is not equal to 0x%02X\n", chip_8_instance.V[(instruction & 0x0F00) >> 8], instruction & 0x00FF);
                break;

            // Skip 1 instruction if Vx and Vy are equal
            case 0x5:
                if (chip_8_instance.V[(instruction & 0x0F00) >> 8] == chip_8_instance.V[(instruction & 0x00F0) >> 4]){
                    chip_8_instance.PC += 2;
                }
                //printf("Checked if V[%d] = 0x%02X is equal to V[%d] = 0x%02X\n", ((instruction & 0x0F00) >> 8), chip_8_instance.V[(instruction & 0x0F00) >> 8], ((instruction & 0x00F0) >> 4), chip_8_instance.V[(instruction & 0x00F0) >> 4]);
                break;

            // Set Vx to NN
            case 0x6:
                chip_8_instance.V[(instruction & 0x0F00) >> 8] = instruction & 0x00FF;
                //printf("Set V[%d] to 0x%02X\n", (instruction & 0x0F00) >> 8, instruction & 0x00FF);
                break;

            // Add NN to Vx (without carry) and without changing the carry flag
            case 0x7:
                chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x0F00) >> 8] + (instruction & 0x00FF);
                //printf("Add 0x%02X to V[%d], result: 0x%02X\n", instruction & 0x00FF, (instruction & 0x0F00) >> 8, chip_8_instance.V[(instruction & 0x0F00) >> 8]);
                break;

            // 0x8 is used for a lot of logical and arthmetic instructions so we switch for each
            case 0x8:
                switch (instruction & 0x000F){
                    // Binary Set operation
                    case 0x0:
                        chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x00F0) >> 4];
                        //printf("Set V[%d] to equal V[%d]\n", ((instruction & 0x0F00) >> 8), ((instruction & 0x00F0) >> 4));
                        break;

                    // Binary Or operation
                    case 0x1:
                        chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x0F00) >> 8] | chip_8_instance.V[(instruction & 0x00F0) >> 4];
                        //printf("Set V[%d] to the or operation of V[%d] | V[%d]\n", ((instruction & 0x0F00) >> 8), ((instruction & 0x0F00) >> 8), ((instruction & 0x00F0) >> 4));
                        break;

                    // Binary And operation
                    case 0x2:
                        chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x0F00) >> 8] & chip_8_instance.V[(instruction & 0x00F0) >> 4];
                        //printf("Set V[%d] to the and operation of V[%d] & V[%d]\n", ((instruction & 0x0F00) >> 8), ((instruction & 0x0F00) >> 8), ((instruction & 0x00F0) >> 4));
                        break;

                    // Binary XOR operation
                    case 0x3:
                        chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x0F00) >> 8] ^ chip_8_instance.V[(instruction & 0x00F0) >> 4];
                        //printf("Set V[%d] to the XOR operation of V[%d] ^ V[%d]\n", ((instruction & 0x0F00) >> 8), ((instruction & 0x0F00) >> 8), ((instruction & 0x00F0) >> 4));
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
                        break;

                    // Shift V[X] by 1 bit (right) if the bit shifted out was 1 set V[F] to 1 else set it to 0
                    case 0x6:
                        if (chip_8_instance.V[(instruction & 0x0F00) >> 8] & 0b00000001){
                            temporary_u8 = 1;

                        }
                        else{
                            temporary_u8 = 0;
                        }
                        chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x0F00) >> 8] >> 1;
                        chip_8_instance.V[15] = temporary_u8;
                        break;

                    // Shift V[X] by 1 bit (left) if the bit shifted out was 1 set V[F] to 1 else set it to 0
                    case 0xE:
                        if (chip_8_instance.V[(instruction & 0x0F00) >> 8] & 0b10000000){
                            temporary_u8 = 1;

                        }
                        else{
                            temporary_u8 = 0;
                        }
                        chip_8_instance.V[(instruction & 0x0F00) >> 8] = chip_8_instance.V[(instruction & 0x0F00) >> 8] << 1;
                        chip_8_instance.V[15] = temporary_u8;
                        break;

                }
                break;

            // Skip 1 instruction if Vx and Vy are equal
            case 0x9:
                if (chip_8_instance.V[(instruction & 0x0F00) >> 8] != chip_8_instance.V[(instruction & 0x00F0) >> 4]){
                    chip_8_instance.PC += 2;
                }
                //printf("Checked if V[%d] = 0x%02X is not equal to V[%d] = 0x%02X\n", ((instruction & 0x0F00) >> 8), chip_8_instance.V[(instruction & 0x0F00) >> 8], ((instruction & 0x00F0) >> 4), chip_8_instance.V[(instruction & 0x00F0) >> 4]);
                break;

            // Set the index register to NNN
            case 0xA:
                chip_8_instance.I = (instruction & 0x0FFF);
                //printf("Set I to 0x%03X\n", chip_8_instance.I);
                break;

            // Jump with an offset (original implementation)
            case 0xB:
               chip_8_instance.PC = (instruction & 0x0FFF) + chip_8_instance.V[0];
               break;

            // Get a random value (0 - 255 inclusive) then binary and it with the two last nibbles of the instruction
            case 0xC:
               chip_8_instance.V[(instruction & 0x0F00) >> 8] = GetRandomValue(0, 255) & (instruction & 0x00FF);
               break;

            // Case of us writting a sprite to the screen
            case 0xD:
                // DXYN
                // set the X coordinate to the value in VX (V register N number) modulo 64
                x_coordinate = chip_8_instance.V[(instruction & 0x0F00) >> 8] & 63;
                y_coordinate = chip_8_instance.V[(instruction & 0x00F0) >> 4] & 31;
                chip_8_instance.V[15] = 0;

                // For N pixels
                for (int i = 0; i < (instruction & 0x000F); i++){
                    // Grab the sprite data
                    u8 sprite_data = chip_8_instance.memory[chip_8_instance.I + i];
                    // Until we're at the edge or we have reached the end of what to read
                    for(int j = 0; j < 8 && (x_coordinate + j) < 64; j++){
                        // if the bit working from left to right is 1
                        if ((sprite_data & (0x80 >> j)) != 0){
                            // Get the emulated displays index
                            int display_index = y_coordinate * chip_8_screen_width + x_coordinate + j;

                            // If that pixel is 1 then we set it to 0 and set V[F] to 1
                            if (chip_8_instance.display[display_index]){
                                chip_8_instance.display[display_index] = 0;
                                chip_8_instance.V[15] = 1;
                            }
                            // Else we just set that pixel to 1
                            else{
                                chip_8_instance.display[display_index] = 1;
                            }
                        }
                    }
                    // Increment y
                    y_coordinate += 1;
                    // If we're ad the end of our Y max break out
                    if (y_coordinate == 32){
                        break;
                    }

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
                        //printf("Character given was 0x%01X\n", current_key_pressed);
                        if (temporary_u8 == current_key_pressed) {  // check the correct key
                            chip_8_instance.PC += 2;
                        }
                        break;
                    }
                    // Skip if key in Vx is NOT pressed
                    case 0xA1: {
                        temporary_u8 = chip_8_instance.V[(instruction & 0x0F00) >> 8];
                        current_key_pressed = get_most_recent_input(&chip_8_instance);
                        //printf("Character given was 0x%01X\n", current_key_pressed);
                        if (!((temporary_u8 == current_key_pressed) && (temporary_u8 != 0xFF))) {
                            chip_8_instance.PC += 2;
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
                        break;
                    // Set the delay_register (timer) to V[X]
                    case 0x15:
                        chip_8_instance.delay_register = chip_8_instance.V[(instruction & 0x0F00) >> 8];
                        break;
                    // Set the sound_register (timer for sound) to V[X]
                    case 0x18:
                        chip_8_instance.sound_register = chip_8_instance.V[(instruction & 0x0F00) >> 8];
                        break;
                    // Add V[X] to our index register (ambiguous behavior)
                    case 0x1E:
                        chip_8_instance.I += chip_8_instance.V[(instruction & 0x0F00) >> 8];
                        break;
                    // Get a key blocking until a key is recieved
                    case 0x0A:
                        temporary_u8 = get_most_recent_input(&chip_8_instance);
                        if (temporary_u8 != 0xFF){
                            chip_8_instance.V[(instruction & 0xF00) >> 8] = temporary_u8;
                        }
                        else{
                            chip_8_instance.PC -= 2;
                        }
                        break;
                    // Set our index register to the requested font V[X]
                    case 0x29:
                        chip_8_instance.I = 0x50 + (chip_8_instance.V[(instruction & 0x0F00) >> 8] * 5);
                        break;
                    // Binary-Coded decimal conversion i = (V[X] / 100), i + 1 = (V[X] % 100) / 10, i + 2 = (V[X] % 10)
                    // AKA we take each digit of of V[X] and place them individually in I incrementing for each digit
                    case 0x33:
                        temporary_u8 = chip_8_instance.V[(instruction & 0x0F00) >> 8];
                        chip_8_instance.memory[chip_8_instance.I] = temporary_u8 / 100;
                        chip_8_instance.memory[chip_8_instance.I + 1] = (temporary_u8 % 100) / 10;
                        chip_8_instance.memory[chip_8_instance.I + 2] = (temporary_u8 % 10);
                        break;
                    // We change our emulated memory to the registers from 0 to X
                    // AKA overwrite our emulated memory starting at i with V[0] till i + x = V[X]
                    case 0x55:
                        temporary_u8 = ((instruction & 0x0F00) >> 8);
                        for( u8 i = 0; i <= temporary_u8; i++){
                            chip_8_instance.memory[chip_8_instance.I + i] = chip_8_instance.V[i];
                        }
                        break;
                    // We change our emulated registers to the memory from 0 to X
                    // AKA Overwrite our registers starting at V[0] with memory[i] till V[X] = memory[i] + x
                    case 0x65:
                        temporary_u8 = ((instruction & 0x0F00) >> 8);
                        for( u8 i = 0; i <= temporary_u8; i++){
                            chip_8_instance.V[i] = chip_8_instance.memory[chip_8_instance.I + i];
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
        if (debug){
            printf("Enter a command: ");
            fgets(input, sizeof(input), stdin);

            // remove newline if present
            input[strcspn(input, "\n")] = 0;

            if (input[0] == '\0'){
                continue;
            }
            else{
                if (strcmp(input, "print") == 0){
                    print_chip_8_contents(&chip_8_instance);

                }
            }

         }

         // increment our instruction count
         instruction_count = instruction_count + 1;

         // Update the time registers (decrement if its been 1/60 of a second)
         update_time_registers(&chip_8_instance);

         // If it's time to print a frame print the frame and reset our instruction count
         if (time_has_passed(&when_next_frame)){
            // Prep buffer for editing
            BeginDrawing();
            draw_frame(&chip_8_instance);
            // Draw the edited buffer to the screen
            EndDrawing();
            // Update for when we should print another frame
            when_next_frame = make_future_time(16.6667);
            // Reset instruction count
            instruction_count = 0;
         }

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

         //pretty_timer();
         // track this instruction
         track_instructions();
         //printf("%ld\n", time_to_sleep);
         // Sleep with delta time
         sleep_for_instruction(time_per_instruction_ms);



    }

    // Clean up
    // unload our beep audio file
    UnloadSound(beep);
    // Close the audio file
    CloseAudioDevice();
    // Close the window
    CloseWindow();
    return 0;
}
