.bss
stack:
.skip 0x4000
.align 16
stack_top:

.text
.extern arch_init
.global _start
_start:
    mov $stack_top, %rsp
    xor %rbp, %rbp
    call arch_init
1:
    hlt
    jmp 1b
