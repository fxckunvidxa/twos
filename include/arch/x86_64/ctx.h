#pragma once
#include <types.h>

struct arch_kern_ctx {
    uptr ip, sp;
    u64 rbx, rbp, r12, r13, r14, r15;
};

extern int __attribute__((returns_twice)) arch_save_ctx(struct arch_kern_ctx *);
extern void __attribute__((noreturn)) arch_restore_ctx(struct arch_kern_ctx *);