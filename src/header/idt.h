#ifndef _IDT_H
#define _IDT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define IDT_MAX_ENTRY_COUNT 256
#define ISR_STUB_TABLE_LIMIT IDT_MAX_ENTRY_COUNT
#define INTERRUPT_GATE_R_BIT_1 0b000
#define INTERRUPT_GATE_R_BIT_2 0b110
#define INTERRUPT_GATE_R_BIT_3 0b0

extern void *isr_stub_table[ISR_STUB_TABLE_LIMIT];

extern struct IDTR _idt_idtr;

/**
 * IDTGate, IDT entry that point into interrupt handler
 * Struct defined exactly in Intel x86 Vol 3a - Figure 6-2. IDT Gate Descriptors
 *
 * @param offset_low   Lower 16-bit offset
 * @param segment      Memory segment selector
 * @param _ist         Interrupt Stack Table (bits 0-2 of byte 4)
 * @param _reserved_1  Reserved (bits 3-7 of byte 4)
 * @param type         Gate Type (bits 0-3 of byte 5, e.g., 0xE for 32-bit int gate)
 * @param s_bit        Storage Segment (0 for interrupt/trap gates)
 * @param dpl          Descriptor Privilege Level (0-3)
 * @param p_bit        Present bit
 * @param offset_high  Higher 16-bit offset
 */
struct IDTGate
{
    // First 32-bit (Bit 0 to 31)
    uint16_t offset_low;
    uint16_t segment; // Segment selector

    // Next 32-bit (Bit 32 to 63)
    // Byte 4
    uint8_t _ist : 3;        // Interrupt Stack Table
    uint8_t _reserved_1 : 5; // Reserved

    // Byte 5 (Type and Attributes)
    uint8_t type : 4;  // Gate Type (e.g., 0xE = 32-bit int)
    uint8_t s_bit : 1; // Storage Segment (0 for int/trap)
    uint8_t dpl : 2;   // Descriptor Privilege Level
    uint8_t p_bit : 1; // Present

    // Byte 6 & 7
    uint16_t offset_high;
} __attribute__((packed));

/**
 * Interrupt Descriptor Table, containing lists of IDTGate.
 *
 * @param table Array dari IDTGate
 */
struct InterruptDescriptorTable
{
    struct IDTGate table[IDT_MAX_ENTRY_COUNT];
} __attribute__((packed));

/**
 * IDTR, carrying information where's the IDT located and size.
 * Global kernel variable defined at idt.c.
 *
 * @param limit Ukuran IDT dalam bytes - 1
 * @param base  Alamat linear (virtual) dari IDT
 */
struct IDTR
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/**
 * Set IDTGate with proper interrupt handler values.
 * Will directly edit global IDT variable and set values properly
 * * @param int_vector       Interrupt vector to handle
 * @param handler_address  Interrupt handler address
 * @param gdt_seg_selector GDT segment selector, for kernel use GDT_KERNEL_CODE_SEGMENT_SELECTOR
 * @param privilege        Descriptor privilege level
 */
void set_interrupt_gate(uint8_t int_vector, void *handler_address, uint16_t gdt_seg_selector, uint8_t privilege);

/**
 * Set IDT with proper values and load with lidt
 */
void initialize_idt(void);

#endif