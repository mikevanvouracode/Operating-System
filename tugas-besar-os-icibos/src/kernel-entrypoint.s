global loader                        ; the entry symbol for ELF
global load_gdt                      ; load GDT table
global set_tss_register              ; set tss register to GDT entry
extern kernel_setup                  ; kernel C entrypoint
extern _paging_kernel_page_directory ; kernel page directory

KERNEL_VIRTUAL_BASE equ 0xC0000000    ; kernel virtual memory
KERNEL_STACK_SIZE   equ 2097152       ; size of stack in bytes
MAGIC_NUMBER        equ 0x1BADB002    ; define the magic number constant
FLAGS               equ 0x0           ; multiboot flags
CHECKSUM            equ -MAGIC_NUMBER ; calculate the checksum (magic number + checksum + flags == 0)


section .bss
align 4                    ; align at 4 bytes
kernel_stack:              ; label points to beginning of memory
    resb KERNEL_STACK_SIZE ; reserve stack for the kernel


section .multiboot  ; GRUB multiboot header
align 4             ; the code must be 4 byte aligned
    dd MAGIC_NUMBER ; write the magic number to the machine code,
    dd FLAGS        ; the flags,
    dd CHECKSUM     ; and the checksum


; start of the text (code) section
section .setup.text 
loader equ (loader_entrypoint - KERNEL_VIRTUAL_BASE)
loader_entrypoint:         ; the loader label (defined as entry point in linker script)
    ; Set CR3 (CPU page register)
    ; Ambil alamat fisik dari _paging_kernel_page_directory
    mov eax, _paging_kernel_page_directory - KERNEL_VIRTUAL_BASE
    mov cr3, eax

    ; Use 4 MB paging
    mov eax, cr4
    or  eax, 0x00000010    ; PSE (4 MB paging)
    mov cr4, eax

    ; Enable paging
    mov eax, cr0
    or  eax, 0x80000000    ; PG flag
    mov cr0, eax

    ; Lompat ke alamat virtual (Higher Half)
    ; Alamat fisik saat ini akan tidak valid
    lea eax, [loader_virtual]
    jmp eax

loader_virtual:
    ; Hapus identity mapping (Virtual 0 -> Fisik 0)
    ; Kita sudah berada di Higher Half, jadi aman
    mov dword [_paging_kernel_page_directory], 0
    invlpg [0]                                ; Hapus TLB cache untuk halaman 0
    
    ; Setup stack register (ESP) ke alamat virtualnya
    mov esp, kernel_stack + KERNEL_STACK_SIZE 
    
    ; Panggil C kernel
    call kernel_setup
.loop:
    jmp .loop                                 ; loop forever


section .text
; More details: https://en.wikibooks.org/wiki/X86_Assembly/Protected_Mode
load_gdt:
    cli
    mov  eax, [esp+4]
    lgdt [eax] ; Load GDT from GDTDescriptor, eax at this line will point GDTR location
    
    ; Set bit-0 (Protection Enable bit-flag) in Control Register 0 (CR0)
    ; This is optional, as usually GRUB already start with protected mode flag enabled
    mov  eax, cr0
    or   eax, 1
    mov  cr0, eax

    ; Far jump to update cs register
    ; Warning: Invalid GDT will raise exception in any instruction below
    jmp 0x8:flush_cs
flush_cs:
    ; Update all segment register
    mov ax, 10h
    mov ss, ax
    mov ds, ax
    mov es, ax
    ret
global kernel_execute_user_program
kernel_execute_user_program:
    ; Set user data segments
    mov eax, 0x20 | 0x3  ; User data selector + RPL 3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Get entry point from parameter (first arg on stack)
    mov ecx, [esp+4]  ; ecx = entry point (0x0)

    ; Prepare stack for IRET
    push eax           ; SS (user data selector)
    
    mov eax, ecx
    add eax, 0x400000 - 4  ; User stack pointer (top of 4MB)
    push eax           ; ESP
    
    pushf              ; EFLAGS
    
    mov eax, 0x18 | 0x3  ; User code selector + RPL 3
    push eax           ; CS
    
    push ecx           ; EIP (entry point)
    
    iret


set_tss_register:
    mov ax, 0x28 | 0 ; GDT TSS Selector, ring 0
    ltr ax
    ret