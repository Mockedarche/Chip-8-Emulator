#include "chip_8_core.h"
#include <stdio.h>
#include <stdlib.h>
#include "timing.h"


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

// Programs start at memory location 0x200 (512)
const int rom_start_address = 0x200;

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
