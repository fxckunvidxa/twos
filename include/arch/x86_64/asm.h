#pragma once
#include <types.h>

static inline void invlpg(uptr va)
{
    asm volatile("invlpg (%0)" ::"r"(va) : "memory");
}

static inline u64 read_tsc()
{
    u32 lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((u64)hi << 32) | lo;
}

static inline void cpu_pause()
{
    asm volatile("pause");
}

static inline void inf_loop()
{
    while (1)
    {
        asm volatile("cli; hlt");
    }
}

static inline void rdmsr(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
   asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

static inline void wrmsr(uint32_t msr, uint32_t lo, uint32_t hi)
{
   asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}