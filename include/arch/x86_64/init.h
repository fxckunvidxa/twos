#pragma once
#include <types.h>
#include <arch/x86_64/bootinfo.h>

void pm_init(struct efi_mem_info *);
void vm_set_cr3(union pme *pml4);