#pragma once
#include <types.h>

#define PG_PRSNT (1 << 0)
#define PG_WR (1 << 1)
#define PG_USR (1 << 2)
#define PG_WRTHR (1 << 3)
#define PG_CDIS (1 << 4)
#define PG_LARGE (1 << 7)
#define PG_FRAME (1 << 9)
#define PG_XD (1 << 63)

#define KERNEL_START 0xFFFFFFFF80000000 /* -2 GiB */
#define KHEAP_START (KERNEL_START + 0x1000000) /* KERNEL_START + 16 MiB */
#define VM_PHYS_MAP_START 0xffff800000000000 /* -128 TiB */
#define VM_MR_KSTACK_START 0xffffffff40000000 /* -3 GiB */
#define VM_MR_KSTACK_END 0xffffffff7ffff000 /* -2 GiB - 4 KiB */

#define VIRT(x) ((uptr)(x) | VM_PHYS_MAP_START)
#define PHYS(x) ((uptr)(x) & ~VM_PHYS_MAP_START)

struct va_bits {
    usize pg_off : 12;
    usize pt : 9;
    usize pd : 9;
    usize pdpt : 9;
    usize pml4 : 9;
    usize __c : 16;
} __attribute__((packed));

struct pg_bits {
    usize prsnt : 1;
    usize write : 1;
    usize user : 1;
    usize wrthr : 1;
    usize nocache : 1;
    usize acsd : 1;
    usize drt : 1;
    usize size : 1;
    usize glb : 1;
    usize frame : 1;
    usize avl0 : 2;
    usize addr : 40;
    usize avl1 : 7;
    usize pk : 4;
    usize xd : 1;
} __attribute__((packed));