#pragma once
#include <types.h>

struct gdt_ent
{
    u16 limit_low;
    u16 base_low;
    u8 base_mid;
    u8 access;
    u8 gran;
    u8 base_high;
} __attribute__((packed));

struct gdtr
{
    u16 limit;
    u64 base;
} __attribute__((packed));

void gdt_init();