#define PRINT_FMT(x) "arch_init: " x
#include <arch/x86_64/acpi.h>
#include <arch/x86_64/asm.h>
#include <arch/x86_64/bootinfo.h>
#include <arch/x86_64/ehci.h>
#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/init.h>
#include <arch/x86_64/irq.h>
#include <arch/x86_64/isr.h>
#include <arch/x86_64/pci.h>
#include <arch/x86_64/pic.h>
#include <arch/x86_64/pit.h>
#include <arch/x86_64/ps2dkbd.h>
#include <arch/x86_64/rtc.h>
#include <arch/x86_64/xhci.h>
#include <fb.h>
#include <main.h>
#include <mt.h>
#include <panic.h>
#include <printk.h>
#include <time.h>
#include <vm.h>
#include <vt.h>

struct bootinfo *g_bi;

void arch_init(struct bootinfo *bi)
{
    union pme *pml4;
    asm volatile("mov %%cr3, %0" : "=r"(pml4));
    pml4[0].raw = 0;
    vm_set_cr3(pml4);

    g_bi = bi;
    gdt_init();
    isr_init();
    idt_init();

    // vm_munmap(0, 1);

    pm_init(&bi->efi_mem);

    struct generic_fb efifb;
    efifb.base = VIRT(bi->efi_gop.base);
    efifb.size = bi->efi_gop.size;
    efifb.width = bi->efi_gop.width;
    efifb.height = bi->efi_gop.height;
    efifb.pitch = bi->efi_gop.pitch;
    fb_set(&efifb);

    vt_init();

    print("EFI GOP framebuffer %lux%lu 32 bpp at 0x%llx\n", efifb.width, efifb.height, bi->efi_gop.base);

    usize pages = 0;
    for (usize i = 0; i < bi->efi_mem.no_entries; i++) {
        struct efi_memory_descriptor *d = (uptr)bi->efi_mem.map + i * bi->efi_mem.desc_size;
        if (d->type == EfiBootServicesCode || d->type == EfiConventionalMemory || d->type == EfiLoaderCode) {
            pages += d->number_of_pages;
        }
    }
    print("Usable memory: %llu KiB (%llu MiB)\n", pages * 4, pages / 256);

    vm_map_mmio(0xFEE00000, 1);

    pic_init();
    irq_init();
    acpi_init(bi->acpi2_rsdp);
    pit_init();
    time_init();
    ps2dkbd_init();
    mt_init();
    // pci_list();

    asm volatile("sti");

    xhci_init();
    ehci_init();

    kmain();
}

void arch_reboot()
{
    asm volatile("cli");
    g_bi->efi_rt.reset_system(EFI_RESET_COLD, 0, 0, NULL);
    panic("arch_reboot via EFI runtime failed");
    inf_loop();
}

void arch_poweroff()
{
    asm volatile("cli");
    g_bi->efi_rt.reset_system(EFI_RESET_SHUTDOWN, 0, 0, NULL);
    panic("arch_poweroff via EFI runtime failed");
    inf_loop();
}
