global _start
extern main

section .text
_start:
    call main
    
    ; Exit syscall (syscall 10)
    mov eax, 10
    mov ebx, 0
    int 0x30
    
    ; Fallback: infinite loop
    cli
    hlt
    jmp $