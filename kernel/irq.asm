global irq0
global irq1
global apic_timer

extern irq0_real
extern irq1_real
extern apic_timer_real

section .text

irq0:
    jmp irq0_real

irq1:
    jmp irq1_real

apic_timer:
    jmp apic_timer_real

section .note.GNU-stack noalloc noexec nowrite progbits

