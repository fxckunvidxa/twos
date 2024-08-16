#pragma once
#include <arch/x86_64/vm.h>
#include <pm.h>
#include <types.h>

#define PAGE_SIZE 0x1000
#define PAGE_SIZE_2M 0x200000
#define PALIGN_UP(x) (((usize)(x) + PAGE_SIZE - 1) & ~0xfff)
#define PALIGN_DOWN(x) ((usize)(x) & ~0xfff)
#define PG_IDX(x) ((usize)(x) >> 12)
#define PG_IDX_UP(x) PG_IDX(PALIGN_UP(x))

#define VM_PG_WR (1 << 0)
#define VM_PG_USR (1 << 1)
#define VM_PG_WT (1 << 2)
#define VM_PG_NC (1 << 3)
#define VM_PG_2M (1 << 4)
#define VM_PG_FR (1 << 5)
#define VM_PG_XD (1 << 6)

#define VM_GP_ALLOC (1 << 0)
#define VM_GP_2M (1 << 1)
#define VM_GP_ONLY_4K (1 << 2)

union pme {
    struct pg_bits data;
    usize raw;
};

enum vm_map_region { VM_MR_KHEAP, VM_MR_KSTACK };

void vm_set_flags(uptr virt, usize num, usize flags);
uptr vm_mmap(enum vm_map_region reg, uptr start, uptr *ph_arr, usize num);
bool vm_munmap(uptr start, usize num);
uptr vm_map_mmio(uptr phys_start, usize num);