#pragma once
#include <types.h>

void irq_init();
void irq_set_handler(u8 num, uptr hnd);