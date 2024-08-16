#include "elf64.h"
#include "ldr_util.h"

EFI_STATUS __attribute__((noreturn)) efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);

    Print(L"TinkyWinkyOS loader v2024.08\r\n");

    EFI_LOADED_IMAGE *LdImg;
    uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle, &LoadedImageProtocol, &LdImg);

    EFI_FILE_HANDLE Root = LibOpenRoot(LdImg->DeviceHandle);

    Elf64_Ehdr *Ehdr = LdrLoadFile(Root, L"\\twos\\kernel", NULL);
    if (CompareMem(Ehdr->e_ident, ELFMAG, SELFMAG) != 0 || Ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
        Ehdr->e_ident[EI_DATA] != ELFDATA2LSB || Ehdr->e_type != ET_EXEC || Ehdr->e_version != EV_CURRENT ||
        Ehdr->e_machine != EM_X86_64) {
        Print(L"Error: bad ELF\r\n");
        LdrExit();
    }

    struct bootinfo *BootInfo = AllocatePool(sizeof(struct bootinfo));
    for (int i = 0; i < 16; i++) {
        CHAR16 *Path = PoolPrint(L"\\twos\\data\\%u", i);
        BootInfo->data[i].base = LdrLoadFile(Root, Path, &BootInfo->data[i].size);
        FreePool(Path);
        if (!BootInfo->data[i].base) {
            break;
        }
    }

    uefi_call_wrapper(Root->Close, 1, Root);

    BootInfo->acpi2_rsdp = NULL;
    for (UINTN i = 0; i < ST->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE *Ent = &ST->ConfigurationTable[i];
        if (CompareGuid(&Ent->VendorGuid, &(EFI_GUID)ACPI_20_TABLE_GUID) == 0) {
            BootInfo->acpi2_rsdp = Ent->VendorTable;
            break;
        }
    }

    union PME *BootPML4 = LdrAllocZrFrame();
    Print(L"loading kernel...\r\n");

    Elf64_Phdr *Phdrs = (UINT8 *)Ehdr + Ehdr->e_phoff;
    for (UINTN i = 0; i < Ehdr->e_phnum; i++) {
        Elf64_Phdr *Phdr = (UINT8 *)Phdrs + i * Ehdr->e_phentsize;
        if (Phdr->p_type == PT_LOAD) {
            Print(L"PT_LOAD offset=0x%lx filesz=0x%lx vaddr=0x%lx memsz=0x%lx flags=", Phdr->p_offset, Phdr->p_filesz,
                  Phdr->p_vaddr, Phdr->p_memsz);
            UINTN Flags = 0;
            if (Phdr->p_flags & PF_R) {
                Print(L"R");
            }
            if (Phdr->p_flags & PF_W) {
                Print(L"W");
                Flags |= PG_WR;
            }
            if (!(Phdr->p_flags & PF_X)) {
                Flags |= PG_XD;
            } else {
                Print(L"X");
            }
            Print(L"\r\n");
            if (Phdr->p_vaddr & 0xfff) {
                Print(L"Error: segment base not aligned to page boundary\r\n");
                LdrExit();
            }
            EFI_PHYSICAL_ADDRESS Addr = LdrAllocPages(EFI_SIZE_TO_PAGES(Phdr->p_memsz));
            CopyMem(Addr, (UINT8 *)Ehdr + Phdr->p_offset, Phdr->p_filesz);
            ZeroMem(Addr + Phdr->p_filesz, Phdr->p_memsz - Phdr->p_filesz);
            for (UINTN Phys = Addr; Phys < PALIGN_UP(Addr + Phdr->p_memsz); Phys += EFI_PAGE_SIZE) {
                LdrMapPage(BootPML4, Phdr->p_vaddr + Phys - Addr, Phys, FALSE, Flags);
            }
        }
    }
    void (*KEntry)(struct bootinfo *) = Ehdr->e_entry;
    FreePool(Ehdr);

    LdrSetupGraphics(&BootInfo->efi_gop);

    UINTN NoEnt, MapKey, DescSz;
    UINT32 DescVer;
    EFI_MEMORY_DESCRIPTOR *Map = LibMemoryMap(&NoEnt, &MapKey, &DescSz, &DescVer);

    for (UINTN i = 0; i < NoEnt; i++) {
        EFI_MEMORY_DESCRIPTOR *Desc = (UINT8 *)Map + i * DescSz;
        for (UINTN j = 0; j < Desc->NumberOfPages; j++) {
            LdrMapPage(BootPML4, VM_PHYS_MAP_START + Desc->PhysicalStart + j * EFI_PAGE_SIZE,
                       Desc->PhysicalStart + j * EFI_PAGE_SIZE, FALSE, PG_WR);
        }
    }

    FreePool(Map);

    Map = LibMemoryMap(&NoEnt, &MapKey, &DescSz, &DescVer);
    BootInfo->efi_mem.map = (UINT8 *)Map + VM_PHYS_MAP_START;
    BootInfo->efi_mem.no_entries = NoEnt;
    BootInfo->efi_mem.desc_size = DescSz;

    if (EFI_ERROR(uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, MapKey))) {
        Print(L"ExitBootServices error\r\n");
        LdrExit();
    }

    asm volatile("cli");

    const union PME *EFIPML4;
    asm volatile("mov %%cr3, %0" : "=r"(EFIPML4));
    BootPML4[0].raw = EFIPML4[0].raw;
    asm volatile("mov %0, %%cr3" ::"r"(BootPML4));

    for (UINTN i = 0; i < NoEnt; i++) {
        EFI_MEMORY_DESCRIPTOR *Desc = (UINT8 *)Map + i * DescSz;
        Desc->VirtualStart = VM_PHYS_MAP_START + Desc->PhysicalStart;
    }

    if (EFI_ERROR(uefi_call_wrapper(RT->SetVirtualAddressMap, 4, NoEnt * DescSz, DescSz, DescVer, Map))) {
        uefi_call_wrapper(RT->ResetSystem, 4, EfiResetCold, 0, 0, NULL);
    }

    BootInfo->efi_rt.reset_system = ST->RuntimeServices->ResetSystem;

    KEntry((UINT8 *)BootInfo + VM_PHYS_MAP_START);

    while (1) {
        asm volatile("cli; hlt");
    }
}
