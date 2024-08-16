#pragma once
#ifdef EFI_BOOTINFO
typedef unsigned int u32;
typedef unsigned long long u64, uptr, usize;
#else
#include <types.h>
enum efi_memory_type {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiMaxMemoryType
};
#endif

struct efi_memory_descriptor {
    u32 type;
    u32 pad;
    uptr physical_start;
    uptr virtual_start;
    u64 number_of_pages;
    u64 attribute;
};

struct efi_memory_info {
    usize no_entries, desc_size;
    struct efi_memory_descriptor *map;
};

struct efi_gop_info {
    uptr base;
    usize size;
    u32 width, height, pitch;
};

enum efi_reset_type { EFI_RESET_COLD, EFI_RESET_WARM, EFI_RESET_SHUTDOWN };

struct efi_runtime {
    __attribute__((ms_abi)) void (*reset_system)(enum efi_reset_type type, u64 status, u64 data_size, uptr reset_data);
};

struct bootinfo_data_entry {
    uptr base;
    usize size;
};

struct bootinfo {
    struct efi_gop_info efi_gop;
    struct efi_memory_info efi_mem;
    struct efi_runtime efi_rt;
    struct bootinfo_data_entry data[16];
    uptr acpi2_rsdp;
};
