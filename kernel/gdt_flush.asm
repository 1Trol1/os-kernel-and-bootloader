global gdt_flush
global tss_flush

section .text

gdt_flush:
    lgdt [rdi]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    push 0x08
    lea rax, [rel .reload_cs]
    push rax
    retfq

.reload_cs:
    ret

tss_flush:
    mov ax, di
    ltr ax
    ret

section .note.GNU-stack noalloc noexec nowrite progbits

