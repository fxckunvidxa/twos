#pragma once
#include <types.h>

struct idt_ent
{
    u16 base_low;
    u16 cs;
    u8 ist;
    u8 attr;
    u16 base_mid;
    u32 base_high;
    u32 zero;
} __attribute__((packed));

struct idtr
{
    u16 limit;
    u64 base;
} __attribute__((packed));

struct intr_frame
{
    u64 r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
    u64 int_no, err_code;
    u64 rip, cs, rflags, rsp, ss;
};

void idt_init();
void idt_set_entry(u8 num, uptr hnd, u8 flags, u16 cs);