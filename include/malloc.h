#pragma once
#include <types.h>

void* kmalloc(usize);
void kfree(void *);
void *kcalloc(usize, usize);
void *krealloc(void *, usize);

static inline void *kzalloc(usize s)
{
    return kcalloc(1, s);
}