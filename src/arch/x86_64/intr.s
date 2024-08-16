.text

.irp i, 0, 1, 2, 3, 4, 5, 6, 7, 9, 15, 16, 18, 19, 20, 22, 23, 24, 25, 26, 27, 28, 31
isr\i:
    pushq $0
    pushq $\i
    jmp isr_common
.endr

.irp i, 8, 10, 11, 12, 13, 14, 17, 21, 29, 30
isr\i:
    pushq $\i
    jmp isr_common
.endr

.irp i, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
irq\i:
    pushq $0
    pushq $32+\i
    jmp irq_common
.endr

.macro rsave
push %rax
push %rbx
push %rcx
push %rdx
push %rsi
push %rdi
push %rbp
push %r8
push %r9
push %r10
push %r11
push %r12
push %r13
push %r14
push %r15
.endm

.macro rrstor
pop %r15
pop %r14
pop %r13
pop %r12
pop %r11
pop %r10
pop %r9
pop %r8
pop %rbp
pop %rdi
pop %rsi
pop %rdx
pop %rcx
pop %rbx
pop %rax
.endm

.extern isr_handler
isr_common:
    rsave
    mov %rsp, %rdi
    call isr_handler
    rrstor
    add $16, %rsp
    iretq

.extern irq_handler
irq_common:
    rsave
    mov %rsp, %rdi
    call irq_handler
    rrstor
    add $16, %rsp
    iretq

.data
.global isr_stubs
isr_stubs:
    .irp i, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    .quad isr\i
    .endr

.global irq_stubs
irq_stubs:
    .irp i, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
    .quad irq\i
    .endr
