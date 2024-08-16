#pragma once
#include <types.h>

#define PM_TMR_FREQ 3579545

void acpipmt_init();
bool acpipmt_is_supported();
u32 acpipmt_read();
u32 acpipmt_get_mask();