global _start
extern kernel_main

section .text
_start:
    ; ustawiamy stos (tymczasowy, 16 KB)
    mov     rsp, stack_top

    ; wywołujemy kernel_main()
    call    kernel_main

.hang:
    hlt
    jmp     .hang

section .bss
align 16
stack_bottom:
    resb    16384          ; 16 KB stosu
stack_top:
section .note.GNU-stack noalloc noexec nowrite progbits

