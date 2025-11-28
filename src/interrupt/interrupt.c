#include "header/interrupt.h"
#include "header/portio.h"
#include "header/framebuffer.h"
#include "header/keyboard.h"
#include "header/ext2.h"
#include "header/stdlib/string.h"

// Variabel global TSS
struct TSSEntry _interrupt_tss_entry = {
    .ss0 = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
    .esp0 = 0,
};

void set_tss_kernel_current_stack(void)
{
    uint32_t stack_ptr;
    __asm__ volatile("mov %%ebp, %0" : "=r"(stack_ptr));
    _interrupt_tss_entry.esp0 = stack_ptr;
}

void io_wait(void) { out(0x80, 0); }
void pic_ack(uint8_t irq)
{
    if (irq >= 8)
        out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}
void pic_remap(void)
{
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET);
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET);
    io_wait();
    out(PIC1_DATA, 0b0100);
    io_wait();
    out(PIC2_DATA, 0b0010);
    io_wait();
    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK);
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}
void activate_keyboard_interrupt(void)
{
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_KEYBOARD));
}

void syscall_handler(struct InterruptFrame frame)
{
    uint32_t service_number = frame.cpu.general.eax;
    uint32_t arg1 = frame.cpu.general.ebx;
    uint32_t arg2 = frame.cpu.general.ecx;
    uint32_t arg3 = frame.cpu.general.edx;

    switch (service_number)
    {
    case 0: // read()
        // FIX: Pass pointer langsung (JANGAN dereference dengan *)
        *((int8_t *)arg2) = read((struct EXT2DriverRequest *)arg1);
        break;

    case 1: // read_directory()
        *((int8_t *)arg2) = read_directory((struct EXT2DriverRequest *)arg1);
        break;

    case 2: // write()
        // FIX: Pass pointer langsung (JANGAN dereference dengan *)
        *((int8_t *)arg2) = write((struct EXT2DriverRequest *)arg1);
        break;

    case 3: // delete()
        // FIX: Pass pointer langsung (JANGAN dereference dengan *)
        *((int8_t *)arg2) = delete((struct EXT2DriverRequest *)arg1);
        break;

    case 4: // getchar()
        get_keyboard_buffer((char *)arg1);
        break;

    case 5: // putchar()
        puts((char *)&arg1, 1, (uint8_t)arg2);
        break;

    case 6: // puts()
        puts((const char *)arg1, arg2, (uint8_t)arg3);
        break;

    case 7: // activate_keyboard
        keyboard_state_activate();
        break;

    case 8: // clear_screen
        framebuffer_clear();
        break;

    case 10: // exit
        framebuffer_write_string(24, 0, "Program terminated.", 0xF, 0);
        while (1)
            __asm__("hlt");
        break;

    default:
        break;
    }
}

void main_interrupt_handler(struct InterruptFrame frame)
{
    switch (frame.int_number)
    {
    case 0x8: // Double Fault
        framebuffer_write_string(10, 0, "DOUBLE FAULT!", 0x0C, 0x0);
        __asm__ volatile("cli; hlt");
        break;
    case 0xE: // Page Fault
        framebuffer_write_string(10, 0, "PAGE FAULT!", 0x0C, 0x0);
        __asm__ volatile("cli; hlt");
        break;
    case PIC1_OFFSET + IRQ_KEYBOARD: // 0x21
        keyboard_isr();
        break;
    case 0x30:                  // Syscall
        syscall_handler(frame); // Panggil handler syscall
        break;
    default:
        break;
    }

    if (frame.int_number >= PIC1_OFFSET && frame.int_number <= PIC2_OFFSET + 7)
    {
        pic_ack(frame.int_number - PIC1_OFFSET);
    }
}