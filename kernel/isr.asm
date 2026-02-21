; 64-bit ISR / IRQ / APIC timer stubs — NASM / Intel

default rel

global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31

global irq0_real
global irq1_real
global apic_timer_real

extern isr_handler_c
extern apic_eoi

section .text

; --- makra ISR ---

%macro ISR_NOERR 2
%1:
    push qword 0        ; sztuczny error code
    push qword %2       ; numer przerwania
    jmp isr_common
%endmacro

%macro ISR_ERR 2
%1:
    push qword %2       ; numer przerwania (CPU już pushnął error code)
    jmp isr_common
%endmacro

; --- ISR 0–31 ---

ISR_NOERR isr0,  0
ISR_NOERR isr1,  1
ISR_NOERR isr2,  2
ISR_NOERR isr3,  3
ISR_NOERR isr4,  4
ISR_NOERR isr5,  5
ISR_NOERR isr6,  6
ISR_NOERR isr7,  7

ISR_ERR   isr8,  8

ISR_NOERR isr9,  9
ISR_ERR   isr10, 10
ISR_ERR   isr11, 11
ISR_ERR   isr12, 12
ISR_ERR   isr13, 13
ISR_ERR   isr14, 14

ISR_NOERR isr15, 15
ISR_NOERR isr16, 16
ISR_NOERR isr17, 17
ISR_NOERR isr18, 18
ISR_NOERR isr19, 19
ISR_NOERR isr20, 20
ISR_NOERR isr21, 21
ISR_NOERR isr22, 22
ISR_NOERR isr23, 23
ISR_NOERR isr24, 24
ISR_NOERR isr25, 25
ISR_NOERR isr26, 26
ISR_NOERR isr27, 27
ISR_NOERR isr28, 28
ISR_NOERR isr29, 29
ISR_NOERR isr30, 30
ISR_NOERR isr31, 31

; --- IRQ wewnątrz tego samego pliku (żeby global isr/irq był spójny) ---

irq0_real:
    push qword 0
    push qword 32
    jmp irq_common

irq1_real:
    push qword 0
    push qword 33
    jmp irq_common

apic_timer_real:
    push qword 0
    push qword 0x40
    jmp irq_common

; --- wspólna część ISR ---

isr_common:
    swapgs

    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    call isr_handler_c

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    swapgs
    iretq

; --- wspólna część IRQ ---

irq_common:
    swapgs

    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11

    mov rdi, rsp
    call isr_handler_c

    call apic_eoi

    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    swapgs
    iretq

section .note.GNU-stack noalloc noexec nowrite progbits

