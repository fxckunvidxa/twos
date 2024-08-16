#define PRINT_FMT(x) "ehci: " x
#include <arch/x86_64/ehci.h>
#include <arch/x86_64/pci.h>
#include <printk.h>
#include <vm.h>

void ehci_init_dev(union pci_addr pa, struct pci_common_header *hdr)
{
    if (hdr->class_code != 0x0C || hdr->subclass != 0x03 || hdr->prog_if != 0x20) {
        return;
    }

    usize bar_size;
    uptr bar = pci_read_memory_bar(pa, 0, &bar_size);
    uptr mmio_base = vm_map_mmio(bar, PG_IDX_UP(bar_size));

    print("at PCI %02hhx:%02hhx.%hhx, MMIO base: %016zx (BAR size: %zu KiB)\n", pa.bus, pa.dev, pa.func, bar,
          bar_size / 1024);

    u8 pic_irq = pci_readl(pa, 0x3c) & 0xff;
    print("interrupt line: %hhu\n", pic_irq);

    
}

void ehci_init()
{
    pci_scan(ehci_init_dev);
}