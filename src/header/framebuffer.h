#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <stdint.h>

#define FRAMEBUFFER_WIDTH 80
#define FRAMEBUFFER_HEIGHT 25

extern uint8_t g_framebuffer_row;
extern uint8_t g_framebuffer_col;

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg);
void framebuffer_set_cursor(uint8_t r, uint8_t c);
void framebuffer_clear(void);
void framebuffer_write_string(uint8_t row, uint8_t col, const char *str, uint8_t fg, uint8_t bg);
void framebuffer_write_int(uint8_t row, uint8_t col, int value, uint8_t fg, uint8_t bg);
void framebuffer_write_hex(uint8_t row, uint8_t col, uint8_t val);
void framebuffer_scroll(void);
void puts(const char *str, uint32_t len, uint8_t color);

#endif