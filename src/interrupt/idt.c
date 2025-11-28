#include "header/idt.h"
#include "header/interrupt.h"
#include "header/stdlib/string.h"
#include "header/cpu/gdt.h"

// Mendefinisikan variabel global yang dideklarasikan di idt.h
__attribute__((aligned(0x1000))) struct InterruptDescriptorTable interrupt_descriptor_table;
struct IDTR _idt_idtr;

/**
 * @brief       Mengatur satu entri (gate) di dalam IDT.
 * @param int_vector       Nomor interrupt (0-255)
 * @param handler_address  Alamat dari fungsi handler (dari isr_stub_table)
 * @param gdt_seg_selector Selector segmen GDT (biasanya KERNEL_CODE_SEGMENT_SELECTOR)
 * @param privilege        Tingkat hak akses (0 untuk Kernel, 3 untuk User)
 */
void set_interrupt_gate(
    uint8_t int_vector,
    void *handler_address,
    uint16_t gdt_seg_selector,
    uint8_t privilege)
{
    // Ambil pointer ke entri di dalam tabel
    struct IDTGate *idt_int_gate = &interrupt_descriptor_table.table[int_vector];
    uint32_t handler_addr_32 = (uint32_t)handler_address;

    // Atur offset 16-bit low dan high
    idt_int_gate->offset_low = handler_addr_32 & 0xFFFF;
    idt_int_gate->offset_high = (handler_addr_32 >> 16) & 0xFFFF;

    // Atur selector segmen (selalu GDT Kernel Code)
    idt_int_gate->segment = gdt_seg_selector;

    // Atur byte ke-4 (field _ist dan _reserved_1)
    idt_int_gate->_ist = 0;
    idt_int_gate->_reserved_1 = 0;

    // Atur byte ke-5 (Type and Attributes) sesuai struct di idt.h
    idt_int_gate->p_bit = 1;
    idt_int_gate->dpl = privilege;
    idt_int_gate->s_bit = 0;
    idt_int_gate->type = 0xE;
}

/**
 * @brief Menginisialisasi Interrupt Descriptor Table (IDT).
 * Mengisi IDT dengan 256 handler dari isr_stub_table dan memuatnya ke CPU.
 */
void initialize_idt(void)
{
    // 1. Siapkan pointer IDTR (menggunakan nama struct yang benar dari idt.h)
    _idt_idtr.limit = (sizeof(struct InterruptDescriptorTable)) - 1;
    _idt_idtr.base = (uint32_t)&interrupt_descriptor_table;

    // 2. Kosongkan seluruh tabel IDT untuk keamanan
    memset(&interrupt_descriptor_table, 0, sizeof(struct InterruptDescriptorTable));

    // 3. Isi IDT (256 entri) menggunakan stub dari interrupt.asm
    for (uint16_t i = 0; i < IDT_MAX_ENTRY_COUNT; i++)
    {
        set_interrupt_gate(
            (uint8_t)i,
            isr_stub_table[i],
            GDT_KERNEL_CODE_SEGMENT_SELECTOR,
            0);
    }

    set_interrupt_gate(
        0x30,                 // Nomor interrupt untuk syscall
        isr_stub_table[0x30], // Handler-nya tetap sama
        GDT_KERNEL_CODE_SEGMENT_SELECTOR,
        3 // Privilege 3 (User) - INI YANG PENTING
    );

    // 4. Muat IDT ke register CPU
    __asm__ volatile("lidt (%0)" : : "r"(&_idt_idtr));
}