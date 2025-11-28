#ifndef _GDT_H
#define _GDT_H

#include <stdint.h>

// ==== Constant ====
#define GDT_MAX_ENTRY_COUNT 32
#define GDT_KERNEL_CODE_SEGMENT_SELECTOR 0x08
#define GDT_KERNEL_DATA_SEGMENT_SELECTOR 0x10
#define GDT_USER_CODE_SEGMENT_SELECTOR 0x18
#define GDT_USER_DATA_SEGMENT_SELECTOR 0x20
#define GDT_TSS_SELECTOR 0x28

// Set GDT_TSS_SELECTOR with proper TSS values, accessing _interrupt_tss_entry
void gdt_install_tss(void);

/**
 * GDTR, Global Descriptor Table Register.
 * Struktur 48-bit (16-bit size, 32-bit base)
 */
struct GDTR
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// ==== Segment Descriptor, entri 64-bit GDT. ====

struct SegmentDescriptor
{
    // First 32-bit
    uint16_t segment_low; // Segment Limit bits 0-15
    uint16_t base_low;    // Base Address bits 0-15

    // Next 32-bit (Bit 32 to 63)
    uint8_t base_mid; // Base Address bits 16-23

    // Flags (Byte 5)
    uint8_t type_bit : 4;   // Descriptor Type
    uint8_t non_system : 1; // 1 = Code/Data, 0 = System
    uint8_t privilege : 2;  // Privilege Level (DPL) 0-3
    uint8_t valid_bit : 1;  // Segment Present (P)

    // Flags (Byte 6)
    uint8_t segment_high : 4; // Segment Limit bits 16-19
    uint8_t avl : 1;          // Available
    uint8_t long_mode : 1;    // 64-bit code segment (L-bit)
    uint8_t opr_32_bit : 1;   // Default operation size (D/B bit) (1=32-bit)
    uint8_t granularity : 1;  // Granularity (G-bit) (1=4KB, 0=1B)

    // Byte 7
    uint8_t base_high; // Base Address bits 24-31
} __attribute__((packed));

// ==== GDT Table ====
struct GlobalDescriptorTable
{
    struct SegmentDescriptor table[6]; // Null, KCode, KData, UCode, UData, TSS
} __attribute__((packed));

// --- Variabel Global ---
// Didefinisikan di gdt.c
extern struct GDTR _gdt_gdtr;
extern struct GlobalDescriptorTable global_descriptor_table;

// --- API ---

/**
 * @brief Mengisi entri GDT TSS (index 5) dengan alamat dari _interrupt_tss_entry
 * Sesuai Bab 3.2.2.2 (hlm 126)
 */
void gdt_install_tss(void);

#endif
