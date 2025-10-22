#ifndef chip8_core_h
#define chip8_core_h
#include <sys/time.h>
#include <stdbool.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned char b8;

// Width of the CHIP-8 display
extern const int chip_8_screen_width;

// Height of the CHIP-8 display
extern const int chip_8_screen_height;

// CHIP-8 has 4096 bytes of memory
extern const int memory_size;

// Programs start at memory location 0x200 (512)
extern const int rom_start_address;

// Starting memory address for fonts
#define FONT_START 0x50
#define MEMORY_SIZE 4096

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
void push (stack *emulated_stack, u16 to_push);

/*
 * pop function
 * Expects: Emulated stack to be initialized with a size > 0
 * Does: Pops the top of the stack and returns the u16 value
 *
 */
u16 pop (stack *emulated_stack);

/*
 * chip_8 struct
 * Expects: N/A
 * Does: Defines the structure of the CHIP-8 emulator, including memory, registers, stack, and display
 */
typedef struct chip_8 {

    // The Chip 8's memory
    u8 memory[MEMORY_SIZE];

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

    // display wait timer used to emulate the chip 8s display wait quirk
    u8 display_wait_timer;

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
void update_time_registers(chip_8 *chip_8_object);

/*
 * print_chip_8_contents function
 * Expects: chip 8 object to be initialized correctly
 * Does: Prints out all of the chip 8's emulated hardwares contents (except memory)
 *
 */
int print_chip_8_contents(chip_8 *chip_8_instance);

extern chip_8 chip_8_instance;

#endif /* chip_8_core_h */
