#pragma once
#include <types.h>

uptr pm_alloc_frame();
uptr pm_alloc_zero_frame();
void pm_free_frame(uptr addr);