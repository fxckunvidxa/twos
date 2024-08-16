#pragma once
#include <types.h>

union pci_addr {
    struct {
        u8 off;
        u8 func : 3;
        u8 dev : 5;
        u8 bus;
    } __attribute__((packed));
    u32 raw;
};

#define PCI_ADDR(_bus, _dev, _func) ((union pci_addr) {.bus = _bus, .dev = _dev, .func = _func})

#define PCI_BAR_TYPE(x) (((x) >> 1) & 0b11)
#define PCI_BAR32 0
#define PCI_BAR64 2

struct pci_common_header {
    u16 ven_id, dev_id;
    u16 cmd, status;
    u8 rev_id, prog_if, subclass, class_code;
    u8 cl_size, l_tmr, hdr_type, bist;
} __attribute__((packed));

struct pci_header0_rest {
    u32 bar[6];
    u32 cb_cis;
    u16 ss_vid, ss_id;
    u32 erba;
    u8 cap, reserved1[3];
    u32 reserved2;
    u8 int_ln, int_pin, min_gr, max_lt;
} __attribute__((packed));

struct pci_msix_ent {
    uptr msg_addr;
    u32 msg_data;
    u32 vec_ctrl;
} __attribute__((packed));

void pci_scan(void (*callback)(union pci_addr, struct pci_common_header *));
u32 pci_readl(union pci_addr dev_addr, u8 off);
void pci_writel(union pci_addr dev_addr, u8 off, u32 val);
void pci_read_seq(union pci_addr dev_addr, u32 *buf, u8 start, u8 byte_count);
uptr pci_read_memory_bar(union pci_addr dev_addr, u8 idx, usize *out_sz);