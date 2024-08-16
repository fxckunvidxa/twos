#pragma once
#define EFI_BOOTINFO
#include "../include/arch/x86_64/bootinfo.h"
#include <efi.h>
#include <efilib.h>

#define PG_PRSNT (1 << 0)
#define PG_WR (1 << 1)
#define PG_USR (1 << 2)
#define PG_WRTHR (1 << 3)
#define PG_CDIS (1 << 4)
#define PG_LARGE (1 << 7)
#define PG_FRAME (1 << 9)
#define PG_XD (1 << 63)

#define PALIGN_UP(x) (((UINTN)(x) + EFI_PAGE_SIZE - 1) & ~0xfff)
#define PALIGN_DOWN(x) ((UINTN)(x) & ~0xfff)
#define PG_IDX(x) ((UINTN)(x) >> 12)
#define PG_IDX_UP(x) PG_IDX(PALIGN_UP(x))

#define VM_PHYS_MAP_START 0xffff800000000000

struct va_bits {
    UINTN pg_off : 12;
    UINTN pt : 9;
    UINTN pd : 9;
    UINTN pdpt : 9;
    UINTN pml4 : 9;
    UINTN __c : 16;
} __attribute__((packed));

struct pg_bits {
    UINTN prsnt : 1;
    UINTN write : 1;
    UINTN user : 1;
    UINTN wrthr : 1;
    UINTN nocache : 1;
    UINTN acsd : 1;
    UINTN drt : 1;
    UINTN size : 1;
    UINTN glb : 1;
    UINTN frame : 1;
    UINTN avl0 : 2;
    UINTN addr : 40;
    UINTN avl1 : 7;
    UINTN pk : 4;
    UINTN xd : 1;
} __attribute__((packed));

union PME {
    struct pg_bits data;
    UINTN raw;
};

UINT64 LdrGetFileSz(EFI_FILE_HANDLE FileHandle);
EFI_FILE_HANDLE LdrOpenFile(EFI_FILE_HANDLE Root, CHAR16 *Path);
VOID LdrExit();
EFI_PHYSICAL_ADDRESS LdrAllocPages(UINTN NumPages);
EFI_PHYSICAL_ADDRESS LdrAllocFrame();
EFI_PHYSICAL_ADDRESS LdrAllocZrFrame();
VOID LdrMapPage(union PME *PML4, UINTN Virtual, UINTN Physical, BOOLEAN Page2M, UINTN Flags);
VOID LdrSetupGraphics(struct efi_gop_info *bi_fb);
UINT8 *LdrLoadFile(EFI_FILE_HANDLE Root, CHAR16 *Path, UINTN *OutSz);