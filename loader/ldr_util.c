#include "ldr_util.h"

VOID LdrSetupGraphics(struct efi_gop_info *bi_fb)
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;
    EFI_STATUS Status = BS->LocateProtocol(&GraphicsOutputProtocol, NULL, &GOP);

    if (EFI_ERROR(Status)) {
        Print(u"unable to locate GOP: %r\r\n", Status);
        LdrExit();
    }

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN InfoSz;

    Status = GOP->QueryMode(GOP, GOP->Mode == NULL ? 0 : GOP->Mode->Mode, &InfoSz, &Info);
    if (Status == EFI_NOT_STARTED) {
        Status = GOP->SetMode(GOP, 0);
    }
    if (EFI_ERROR(Status)) {
        Print(u"unable to get current video mode: %r\r\n", Status);
        LdrExit();
    }

    /*UINT32 NoModes = GOP->Mode->MaxMode, NativeMode = GOP->Mode->Mode;

    Print(u"Available video modes:\r\n");

    for (UINT32 i = 0; i < NoModes; i++) {
        Status = GOP->QueryMode(GOP, i, &InfoSz, &Info);
        if (EFI_ERROR(Status)) {
            Print(u"unable to query mode #%u info: %r\r\n", i, Status);
            continue;
        }
        if (Info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
            Print(u"%lu - %lux%lu\r\n", i, Info->HorizontalResolution, Info->VerticalResolution);
        }
    }

    CHAR16 Buf[5];

enter:
    Input(u"Enter preferred mode: ", Buf, 5);

    UINTN InMode = Atoi(Buf);

    if (InMode != NativeMode) {
        Status = GOP->SetMode(GOP, InMode);

        if (EFI_ERROR(Status)) {
            Print(u"\r\nUnable to set mode: %r\r\n", Status);
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
    EFI_STATUS Status = Root->Open(Root, &FileHandle, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    if (EFI_ERROR(Status)) {
        Print(u"unable to open file '%s': %r\r\n", Path, Status);
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
    if (OutSz) {
        *OutSz = Sz;
    }
    UINT8 *FileBuf = AllocatePool(Sz);
    File->Read(File, &Sz, FileBuf);
    File->Close(File);
    Print(u"'%s' (%lu bytes) loaded to 0x%lx\r\n", Path, Sz, FileBuf);
    return FileBuf;
}

VOID __attribute__((noreturn)) LdrExit()
{
    Print(u"Press any key to exit...");
    Pause();
    Exit(EFI_SUCCESS, 0, NULL);
}

EFI_PHYSICAL_ADDRESS LdrAllocPages(UINTN NumPages)
{
    EFI_PHYSICAL_ADDRESS Address;
    EFI_STATUS Status = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, NumPages, &Address);
    if (EFI_ERROR(Status)) {
        Print(u"AllocatePages error: %r\r\n", Status);
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

VOID LdrMapPage(union PME *PML4, UINTN Virtual, UINTN Physical, UINTN Flags)
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

    if (Flags & PG_LARGE) {
        PD[va->pd].raw = Physical | Flags | PG_PRSNT;
        return;
    }

    if (!PD[va->pd].data.prsnt) {
        PD[va->pd].raw = LdrAllocZrFrame() | PG_WR | PG_PRSNT;
    }

    union PME *PT = PD[va->pd].data.addr << 12;
    PT[va->pt].raw = Physical | Flags | PG_PRSNT;
}
