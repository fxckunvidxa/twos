#include "ldr_util.h"

VOID LdrSetupGraphics(struct efi_gop_info *bi_fb)
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;
    EFI_STATUS Status = uefi_call_wrapper(BS->LocateProtocol, 3, &GraphicsOutputProtocol, NULL, &GOP);

    if (EFI_ERROR(Status)) {
        Print(L"unable to locate GOP: %r\r\n", Status);
        LdrExit();
    }

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN InfoSz;

    Status = uefi_call_wrapper(GOP->QueryMode, 4, GOP, GOP->Mode == NULL ? 0 : GOP->Mode->Mode, &InfoSz, &Info);
    if (Status == EFI_NOT_STARTED) {
        Status = uefi_call_wrapper(GOP->SetMode, 2, GOP, 0);
    }
    if (EFI_ERROR(Status)) {
        Print(L"unable to get current video mode: %r\r\n", Status);
        LdrExit();
    }

    /*UINT32 NoModes = GOP->Mode->MaxMode, NativeMode = GOP->Mode->Mode;

    Print(L"Available video modes:\r\n");

    for (UINT32 i = 0; i < NoModes; i++) {
        Status = uefi_call_wrapper(GOP->QueryMode, 4, GOP, i, &InfoSz, &Info);
        if (EFI_ERROR(Status)) {
            Print(L"unable to query mode #%u info: %r\r\n", i, Status);
            continue;
        }
        if (Info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
            Print(L"%lu - %lux%lu\r\n", i, Info->HorizontalResolution, Info->VerticalResolution);
        }
    }

    CHAR16 Buf[5];

enter:
    Input(L"Enter preferred mode: ", Buf, 5);

    UINTN InMode = Atoi(Buf);

    if (InMode != NativeMode) {
        Status = uefi_call_wrapper(GOP->SetMode, 2, GOP, InMode);

        if (EFI_ERROR(Status)) {
            Print(L"\r\nUnable to set mode: %r\r\n", Status);
            goto enter;
        }
    }*/

    bi_fb->base = GOP->Mode->FrameBufferBase;
    bi_fb->size = GOP->Mode->FrameBufferSize;
    bi_fb->width = GOP->Mode->Info->HorizontalResolution;
    bi_fb->height = GOP->Mode->Info->VerticalResolution;
    bi_fb->pitch = GOP->Mode->Info->PixelsPerScanLine * 4;
}

UINT64 LdrGetFileSz(EFI_FILE_HANDLE FileHandle)
{
    EFI_FILE_INFO *FileInfo = LibFileInfo(FileHandle);
    UINT64 Size = FileInfo->FileSize;
    FreePool(FileInfo);
    return Size;
}

EFI_FILE_HANDLE LdrOpenFile(EFI_FILE_HANDLE Root, CHAR16 *Path)
{
    EFI_FILE_HANDLE FileHandle;
    EFI_STATUS Status =
        uefi_call_wrapper(Root->Open, 5, Root, &FileHandle, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    if (EFI_ERROR(Status)) {
        Print(L"unable to open file '%s': %r\r\n", Path, Status);
        return NULL;
    }
    return FileHandle;
}

UINT8 *LdrLoadFile(EFI_FILE_HANDLE Root, CHAR16 *Path, UINTN *OutSz)
{
    EFI_FILE_HANDLE File = LdrOpenFile(Root, Path);
    if (!File) {
        return NULL;
    }
    UINTN Sz = LdrGetFileSz(File);
    UINT8 *FileBuf = AllocatePool(Sz);
    uefi_call_wrapper(File->Read, 3, File, &Sz, FileBuf);
    uefi_call_wrapper(File->Close, 1, File);
    if (OutSz) {
        *OutSz = Sz;
    }
    Print(L"'%s' (%lu bytes) loaded to 0x%lx\r\n", Path, Sz, FileBuf);
    return FileBuf;
}

VOID __attribute__((noreturn)) LdrExit()
{
    Print(L"Press any key to exit...");
    Pause();
    Exit(EFI_SUCCESS, 0, NULL);
}

EFI_PHYSICAL_ADDRESS LdrAllocPages(UINTN NumPages)
{
    EFI_PHYSICAL_ADDRESS Address;
    EFI_STATUS Status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, NumPages, &Address);
    if (EFI_ERROR(Status)) {
        Print(L"AllocatePages error: %r\r\n", Status);
        LdrExit();
    }
    return Address;
}

EFI_PHYSICAL_ADDRESS LdrAllocFrame()
{
    return LdrAllocPages(1);
}

EFI_PHYSICAL_ADDRESS LdrAllocZrFrame()
{
    EFI_PHYSICAL_ADDRESS Address = LdrAllocFrame();
    ZeroMem(Address, EFI_PAGE_SIZE);
    return Address;
}

VOID LdrMapPage(union PME *PML4, UINTN Virtual, UINTN Physical, BOOLEAN Page2M, UINTN Flags)
{
    const struct va_bits *va = &Virtual;

    if (!PML4[va->pml4].data.prsnt) {
        PML4[va->pml4].raw = LdrAllocZrFrame() | PG_WR | PG_PRSNT;
    }

    union PME *PDPT = PML4[va->pml4].data.addr << 12;
    if (!PDPT[va->pdpt].data.prsnt) {
        PDPT[va->pdpt].raw = LdrAllocZrFrame() | PG_WR | PG_PRSNT;
    }

    union PME *PD = PDPT[va->pdpt].data.addr << 12;

    if (Page2M) {
        if (PD[va->pd].data.prsnt) {
            return;
        }
        PD[va->pd].raw = Physical | Flags | PG_LARGE | PG_PRSNT;
        return;
    }

    if (!PD[va->pd].data.prsnt) {
        PD[va->pd].raw = LdrAllocZrFrame() | PG_WR | PG_PRSNT;
    }

    union PME *PT = PD[va->pd].data.addr << 12;
    if (PT[va->pt].data.prsnt) {
        return;
    }
    PT[va->pt].raw = Physical | Flags | PG_PRSNT;
}
