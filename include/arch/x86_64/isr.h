#pragma once
#include <types.h>

void isr_init();
void isr_set_handler(u8 num, uptr hnd);