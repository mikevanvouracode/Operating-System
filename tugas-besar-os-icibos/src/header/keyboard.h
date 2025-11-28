#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

#define KEYBOARD_BUFFER_SIZE 256
#define KEYBOARD_DATA_PORT 0x60
#define IRQ_KEYBOARD 1

/* Keyboard buffer & state */
typedef struct {
    char buffer[KEYBOARD_BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
    bool keyboard_input_on;
} KeyboardState;

extern KeyboardState keyboard_state;

/* Driver interface */
void keyboard_state_activate(void);
void keyboard_state_deactivate(void);
void get_keyboard_buffer(char *c);
void keyboard_isr(void);

#endif
