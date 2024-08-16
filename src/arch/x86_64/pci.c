#include <arch/x86_64/bootinfo.h>
#include <arch/x86_64/pci.h>
#include <arch/x86_64/pio.h>
#include <kctype.h>
#include <kstring.h>
#include <printk.h>
#define PRINT_FMT(x) "pci: " x

#define CFG_ADDR 0xCF8
#define CFG_DATA 0xCFC

extern struct bootinfo *g_bi;

u32 pci_readl(union pci_addr dev_addr, u8 off)
{
    if (off & 0b11) {
        print("readl: invalid offset: %hhu, dev_addr: %08lx\n", off, dev_addr.raw);
        return 0;
    }
    u32 addr = (1u << 31) | dev_addr.raw | off;
    outl(CFG_ADDR, addr);
    return inl(CFG_DATA);
}

void pci_writel(union pci_addr dev_addr, u8 off, u32 val)
{
    if (off & 0b11) {
        print("readl: invalid offset: %hhu, dev_addr: %08lx\n", off, dev_addr.raw);
        return 0;
    }
    u32 addr = (1u << 31) | dev_addr.raw | off;
    outl(CFG_ADDR, addr);
    outl(CFG_DATA, val);
}

void pci_read_seq(union pci_addr dev_addr, u32 *buf, u8 start, u8 byte_count)
{
    if ((start & 0b11) || (byte_count & 0b11)) {
        print("read_seq: invalid input: start=%hhu, byte_count=%hhu, dev_addr: %08lx\n", start, byte_count,
              dev_addr.raw);
        return;
    }
    for (u8 reg = 0; reg < byte_count / 4; reg += 1) {
        buf[reg] = pci_readl(dev_addr, start + reg * 4);
    }
}

uptr pci_read_memory_bar(union pci_addr dev_addr, u8 idx, usize *out_sz)
{
    u32 bar = pci_readl(dev_addr, 0x10 + 0x4 * idx);
    if (bar & 1) {
        print("read_memory_bar: this is an IO bar\n");
        return NULL;
    }
    uptr out_addr = bar & ~0xf;
    if (PCI_BAR_TYPE(bar) == PCI_BAR64) {
        out_addr |= (uptr)pci_readl(dev_addr, 0x10 + 0x4 * (idx + 1)) << 32;
    }
    if (out_sz) {
        pci_writel(dev_addr, 0x4, pci_readl(dev_addr, 0x4) & ~0b11);
        pci_writel(dev_addr, 0x10 + 0x4 * idx, 0xffffffff);
        *out_sz = pci_readl(dev_addr, 0x10 + 0x4 * idx) & ~0xf;
        pci_writel(dev_addr, 0x10 + 0x4 * idx, bar);
        if (PCI_BAR_TYPE(bar) == PCI_BAR64) {
            pci_writel(dev_addr, 0x10 + 0x4 * (idx + 1), 0xffffffff);
            *out_sz |= (uptr)pci_readl(dev_addr, 0x10 + 0x4 * (idx + 1)) << 32;
            pci_writel(dev_addr, 0x10 + 0x4 * (idx + 1), out_addr >> 32);
            *out_sz = ~*out_sz + 1;
        } else {
            *out_sz = (~*out_sz & 0xffffffff) + 1;
        }
        pci_writel(dev_addr, 0x4, pci_readl(dev_addr, 0x4) | 0b11);
    }
    return out_addr;
}

void print_func(union pci_addr pa, struct pci_common_header *h)
{
    print("%02hhx:%02hhx.%hhx %04hx:%04hx ", pa.bus, pa.dev, pa.func, h->ven_id, h->dev_id);

    char hven[5], hdev[5];
    snprintf(hven, 5, "%04hx", h->ven_id);
    snprintf(hdev, 5, "%04hx", h->dev_id);

    const char *line = g_bi->data[0].base;
    const char *end = line + g_bi->data[0].size;

    const char *ven_str = NULL;
    usize ven_slen;

    while (line < end) {
        while ((*line == ' ' || *line == '\n') && line < end) {
            line++;
        }

        if (line == end) {
            break;
        }

        const char *ln_end;
        for (ln_end = line; *ln_end != '\n' && ln_end < end; ln_end++)
            ;

        if (isxdigit(line[0])) {
            if (ven_str) {
                break;
            }
            if (memcmp(line, hven, 4) == 0) {
                ven_slen = ln_end - line - 6;
                ven_str = line + 6;
            }
        } else if (line[0] == '\t' && ven_str && isxdigit(line[1]) && memcmp(line + 1, hdev, 4) == 0) {
            char fmt[32];
            snprintf(fmt, 32, "%%.%us %%.%us\n", ven_slen, ln_end - line - 7);
            printk(fmt, ven_str, line + 7);
            return;
        }
        line = ln_end;
    }
    printk("(Unknown)\n");
}

void pci_scan(void (*callback)(union pci_addr, struct pci_common_header *))
{
    for (u16 bus = 0; bus < 256; bus++) {
        for (u8 dev = 0; dev < 32; dev++) {
            volatile struct pci_common_header hdr;
            pci_read_seq(PCI_ADDR(bus, dev, 0), &hdr, 0, sizeof(hdr));
            if (hdr.ven_id == 0xffff) {
                continue;
            }
            callback(PCI_ADDR(bus, dev, 0), &hdr);
            if (hdr.hdr_type & 0x80) {
                for (u8 func = 1; func < 8; func++) {
                    pci_read_seq(PCI_ADDR(bus, dev, func), &hdr, 0, sizeof(hdr));
                    if (hdr.ven_id == 0xffff) {
                        continue;
                    }
                    callback(PCI_ADDR(bus, dev, func), &hdr);
                }
            }
        }
    }
}

void pci_list()
{
    pci_scan(print_func);
}