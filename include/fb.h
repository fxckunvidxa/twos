#pragma once
#include <types.h>

struct generic_fb {
    uptr base;
    usize size;
    u32 width, height, pitch;
};

struct generic_fb *fb_get();
void fb_set(struct generic_fb *);