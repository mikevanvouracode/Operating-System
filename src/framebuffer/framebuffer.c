#include <stdint.h>
#include <stdbool.h>
#include "header/framebuffer.h"
#include "header/portio.h"

#define FRAMEBUFFER_MEMORY_OFFSET ((uint8_t *)0xC00B8000)
#define FRAMEBUFFER_WIDTH 80
#define FRAMEBUFFER_HEIGHT 25

uint8_t g_framebuffer_row = 0;
uint8_t g_framebuffer_col = 0;

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg)
{
    uint16_t attribute = (bg << 4) | (fg & 0x0F);
    uint16_t offset = (row * FRAMEBUFFER_WIDTH + col) * 2;
    FRAMEBUFFER_MEMORY_OFFSET[offset] = c;
    FRAMEBUFFER_MEMORY_OFFSET[offset + 1] = attribute;
}

void framebuffer_set_cursor(uint8_t row, uint8_t col)
{
    uint16_t pos = row * FRAMEBUFFER_WIDTH + col;
    out(0x3D4, 14);
    out(0x3D5, (uint8_t)(pos >> 8));
    out(0x3D4, 15);
    out(0x3D5, (uint8_t)(pos & 0xFF));
}

void framebuffer_clear(void)
{
    for (uint8_t row = 0; row < FRAMEBUFFER_HEIGHT; row++)
    {
        for (uint8_t col = 0; col < FRAMEBUFFER_WIDTH; col++)
        {
            framebuffer_write(row, col, ' ', 0xF, 0x0);
        }
    }
    // Atur ulang posisi kursor global
    g_framebuffer_row = 0;
    g_framebuffer_col = 0;
    framebuffer_set_cursor(0, 0);
}

void framebuffer_scroll(void)
{
    uint16_t *fb = (uint16_t *)FRAMEBUFFER_MEMORY_OFFSET;
    uint32_t total_cells = FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT;

    for (uint32_t i = 0; i < (total_cells - FRAMEBUFFER_WIDTH); i++)
    {
        fb[i] = fb[i + FRAMEBUFFER_WIDTH];
    }
    uint16_t blank = 0x0F00 | ' ';
    for (uint32_t i = total_cells - FRAMEBUFFER_WIDTH; i < total_cells; i++)
    {
        fb[i] = blank;
    }
    if (g_framebuffer_row >= FRAMEBUFFER_HEIGHT)
    {
        g_framebuffer_row = FRAMEBUFFER_HEIGHT - 1;
    }
}

void framebuffer_write_string(uint8_t row, uint8_t col, const char *str, uint8_t fg, uint8_t bg)
{
    int i = 0;
    while (str[i] != '\0')
    {
        framebuffer_write(row, col + i, str[i], fg, bg);
        i++;
    }
}
void framebuffer_write_hex(uint8_t row, uint8_t col, uint8_t val)
{
    const char *hex = "0123456789ABCDEF";
    uint8_t hi = (val >> 4) & 0xF;
    uint8_t lo = val & 0xF;
    framebuffer_write(row, col, hex[hi], 0xF, 0x0);
    framebuffer_write(row, col + 1, hex[lo], 0xF, 0x0);
}
void framebuffer_write_int(uint8_t row, uint8_t col, int value, uint8_t fg, uint8_t bg)
{
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    bool neg = false;
    if (value == 0)
    {
        framebuffer_write(row, col, '0', fg, bg);
        return;
    }
    if (value < 0)
    {
        neg = true;
        value = -value;
    }
    while (value > 0 && i >= 0)
    {
        buf[i--] = '0' + (value % 10);
        value /= 10;
    }
    if (neg)
        buf[i--] = '-';
    framebuffer_write_string(row, col, &buf[i + 1], fg, bg);
}

/**
 * @brief Menulis string ke posisi kursor global saat ini.
 * Ini adalah fungsi kernel-side yang akan dipanggil oleh syscall 6 (puts)
 * dan syscall 5 (putchar).
 */
void puts(const char *str, uint32_t len, uint8_t color)
{
    for (uint32_t i = 0; i < len; i++)
    {
        char c = str[i];

        if (c == '\n')
        { // --- LOGIKA BARU: Newline ---
            g_framebuffer_col = 0;
            g_framebuffer_row++;
        }
        else if (c == '\b')
        { // --- LOGIKA BARU: Backspace ---
            if (g_framebuffer_col > 0)
            {
                g_framebuffer_col--;
                // Hapus karakter di layar dengan menulis spasi
                framebuffer_write(g_framebuffer_row, g_framebuffer_col, ' ', color, 0);
            }
            else if (g_framebuffer_row > 0)
            {
                // Pindah ke baris sebelumnya
                g_framebuffer_row--;
                g_framebuffer_col = FRAMEBUFFER_WIDTH - 1;
                framebuffer_write(g_framebuffer_row, g_framebuffer_col, ' ', color, 0);
            }
        }
        else
        { // --- Karakter biasa ---
            framebuffer_write(g_framebuffer_row, g_framebuffer_col, c, color, 0);
            g_framebuffer_col++;
        }

        // Handle word wrap (pindah baris jika sudah di kolom 80)
        if (g_framebuffer_col >= FRAMEBUFFER_WIDTH)
        {
            g_framebuffer_col = 0;
            g_framebuffer_row++;
        }

        // Handle scroll (jika sudah di baris 25)
        if (g_framebuffer_row >= FRAMEBUFFER_HEIGHT)
        {
            framebuffer_scroll();
            g_framebuffer_row = FRAMEBUFFER_HEIGHT - 1;
        }
    }
    // Update kursor fisik di layar ke posisi terakhir
    framebuffer_set_cursor(g_framebuffer_row, g_framebuffer_col);
}