#include "header/portio.h"

void out(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t in(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void out16(uint16_t port, uint16_t data) {
    __asm__(
        "outw %0, %1"
        : // <Empty output operand>
        : "a"(data), "Nd"(port)
    );
}

uint16_t in16(uint16_t port) {
    uint16_t result;
    __asm__ volatile(
        "inw %1, %0"
        : "=a"(result)
        : "Nd"(port)
    );
    return result;
}

