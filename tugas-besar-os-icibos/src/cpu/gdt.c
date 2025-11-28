#include "header/cpu/gdt.h"
#include "header/interrupt.h" // Diperlukan untuk TSSEntry

/**
 * global_descriptor_table, GDT yang sudah didefinisikan.
 * 0: Null Descriptor
 * 1: Kernel Code Segment (Ring 0)
 * 2: Kernel Data Segment (Ring 0)
 * 3: User Code Segment (Ring 3)
 * 4: User Data Segment (Ring 3)
 * 5: Task State Segment (TSS)
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        // 0. Null Descriptor
        {
            .segment_low = 0,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0,
            .non_system = 0,
            .privilege = 0,
            .valid_bit = 0,
            .segment_high = 0,
            .avl = 0,
            .long_mode = 0,
            .opr_32_bit = 0,
            .granularity = 0,
            .base_high = 0},

        // 1. Kernel Code Segment (Ring 0)
        {
            .segment_low = 0xFFFF,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0xA, // 0b1010 = Code, Executable, Readable
            .non_system = 1, // 1 = Code/Data
            .privilege = 0,  // 0 = Ring 0 (Kernel)
            .valid_bit = 1,  // 1 = Present
            .segment_high = 0xF,
            .avl = 0,
            .long_mode = 0,
            .opr_32_bit = 1,  // 1 = 32-bit
            .granularity = 1, // 1 = 4KB Granularity
            .base_high = 0},

        // 2. Kernel Data Segment (Ring 0)
        {
            .segment_low = 0xFFFF,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0x2, // 0b0010 = Data, Read/Write
            .non_system = 1,
            .privilege = 0,
            .valid_bit = 1,
            .segment_high = 0xF,
            .avl = 0,
            .long_mode = 0,
            .opr_32_bit = 1,
            .granularity = 1,
            .base_high = 0},

        // 3. User Code Segment (Ring 3) - BARU
        {
            .segment_low = 0xFFFF,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0xA, // 0b1010 = Code, Executable, Readable
            .non_system = 1,
            .privilege = 3, // 3 = Ring 3 (User)
            .valid_bit = 1,
            .segment_high = 0xF,
            .avl = 0,
            .long_mode = 0,
            .opr_32_bit = 1,
            .granularity = 1,
            .base_high = 0},

        // 4. User Data Segment (Ring 3) - BARU
        {
            .segment_low = 0xFFFF,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0x2, // 0b0010 = Data, Read/Write
            .non_system = 1,
            .privilege = 3, // 3 = Ring 3 (User)
            .valid_bit = 1,
            .segment_high = 0xF,
            .avl = 0,
            .long_mode = 0,
            .opr_32_bit = 1,
            .granularity = 1,
            .base_high = 0},

        // 5. Task State Segment (TSS) - BARU
        {
            .segment_low = sizeof(struct TSSEntry),
            .base_low = 0,   // Akan diisi oleh gdt_install_tss()
            .base_mid = 0,   // Akan diisi oleh gdt_install_tss()
            .type_bit = 0x9, // 0b1001 = 32-bit TSS (Available)
            .non_system = 0, // 0 = System
            .privilege = 0,  // DPL 0 (Kernel)
            .valid_bit = 1,  // 1 = Present
            .segment_high = 0,
            .avl = 0,
            .long_mode = 0,
            .opr_32_bit = 1,  // 1 = 32-bit
            .granularity = 0, // 0 = Byte Granularity
            .base_high = 0    // Akan diisi oleh gdt_install_tss()
        }}};

/**
 * _gdt_gdtr, pointer ke GDT
 */
struct GDTR _gdt_gdtr = {
    .limit = sizeof(global_descriptor_table) - 1,
    .base = (uint32_t)&global_descriptor_table};

/**
 * @brief Mengatur alamat _interrupt_tss_entry ke GDT entry ke-5
 */
void gdt_install_tss(void)
{
    uint32_t base = (uint32_t)&_interrupt_tss_entry;

    global_descriptor_table.table[5].base_low = base & 0xFFFF;
    global_descriptor_table.table[5].base_mid = (base >> 16) & 0xFF;
    global_descriptor_table.table[5].base_high = (base >> 24) & 0xFF;
}