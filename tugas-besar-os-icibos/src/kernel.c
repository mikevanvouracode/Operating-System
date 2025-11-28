#include "header/keyboard.h"
#include "header/framebuffer.h"
#include "header/cpu/gdt.h"
#include "header/interrupt.h"
#include "header/portio.h"
#include "header/kernel-entrypoint.h"
#include "header/idt.h"
#include "header/disk.h"
#include "header/ext2.h"
#include "header/memory/paging.h" // Diperlukan untuk Paging
#include <stdint.h>
#include "header/stdlib/string.h" // Diperlukan untuk memset

/**
 * kernel_setup
 * Ini adalah entry point C kernel.
 * Paging sudah diaktifkan oleh kernel-entrypoint.s sebelum fungsi ini dipanggil.
 * Fungsi ini sekarang bertugas untuk meluncurkan program 'shell' di User Mode.
 */
void kernel_setup(void)
{
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);

    /* =================== GDT & INTERRUPTS =================== */
    load_gdt(&_gdt_gdtr);
    initialize_idt();
    pic_remap();
    activate_keyboard_interrupt();
    __asm__ volatile("sti");

    /* =================== FILESYSTEM =================== */
    initialize_filesystem_ext2();

    /* =================== LAUNCHING USER MODE =================== */
    gdt_install_tss();
    set_tss_register();

    if (!paging_allocate_user_page_frame(&_paging_kernel_page_directory, (void *)0x0))
    {
        framebuffer_write_string(0, 0, "FATAL: Memory allocation failed!", 0xC, 0x0);
        while (1)
            ;
    }

    struct EXT2DriverRequest request = {
        .buffer_size = 0x100000,
        .buf = (void *)0x0,
        .parent_inode = 2,
        .name = "shell",
        .name_len = 5,
    };

    int8_t read_status = read(&request);
    if (read_status != 0)
    {
        framebuffer_write_string(4, 0, "Gagal read 'shell'", 0xC, 0x0);
        __asm__ volatile("cli; hlt");
    }

    set_tss_kernel_current_stack();
    kernel_execute_user_program((void *)0x0);

    while (true)
        ;
}