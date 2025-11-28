#ifndef PORTIO_H
#define PORTIO_H

#include <stdint.h>

void out(uint16_t port, uint8_t val);
uint8_t in(uint16_t port);
void out16(uint16_t port, uint16_t data);
uint16_t in16(uint16_t port);

#endif
