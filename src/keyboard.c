#include "header/keyboard.h"
#include "header/portio.h"
#include "header/interrupt.h"
#define IRQ_KEYBOARD 1
KeyboardState keyboard_state = { .head = 0, .tail = 0, .keyboard_input_on = false };

void keyboard_state_activate(void)   { keyboard_state.keyboard_input_on = true; }
void keyboard_state_deactivate(void) { keyboard_state.keyboard_input_on = false; }

void get_keyboard_buffer(char *c) {
    if (keyboard_state.head == keyboard_state.tail) {
        *c = 0; 
        return;
    }
    *c = keyboard_state.buffer[keyboard_state.tail];
    keyboard_state.tail = (keyboard_state.tail + 1) % KEYBOARD_BUFFER_SIZE;
}

static const char keyboard_scancode_1_to_ascii_map[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',  
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',     
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`', 0,        
    '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' '  
};

void keyboard_isr(void) {
    uint8_t scancode = in(KEYBOARD_DATA_PORT);  

    if ((scancode & 0x80) == 0 && keyboard_state.keyboard_input_on) {
        char c = keyboard_scancode_1_to_ascii_map[scancode];
        if (c) {
            keyboard_state.buffer[keyboard_state.head] = c;
            keyboard_state.head = (keyboard_state.head + 1) % KEYBOARD_BUFFER_SIZE;
        }
    }
}
