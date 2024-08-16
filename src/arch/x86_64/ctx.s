.text
.global arch_save_ctx
arch_save_ctx:
    mov (%rsp), %rax
    mov %rax, (%rdi)
    lea 8(%rsp), %rax
    mov %rax, 8(%rdi)
    mov %rbx, 16(%rdi)
    mov %rbp, 24(%rdi)
    mov %r12, 32(%rdi)
    mov %r13, 40(%rdi)
    mov %r14, 48(%rdi)
    mov %r15, 56(%rdi)
    xor %rax, %rax
    ret

.global arch_restore_ctx
arch_restore_ctx:
    mov 8(%rdi), %rsp
    mov 16(%rdi), %rbx
    mov 24(%rdi), %rbp
    mov 32(%rdi), %r12
    mov 40(%rdi), %r13
    mov 48(%rdi), %r14
    mov 56(%rdi), %r15
    mov $1, %rax
    jmp *(%rdi)

.global arch_kthread_init
arch_kthread_init:
    pop %rdi
    ret
