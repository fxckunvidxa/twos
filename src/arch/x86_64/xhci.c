#define PRINT_FMT(x) "xhci: " x
#include <arch/x86_64/asm.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/irq.h>
#include <arch/x86_64/pci.h>
#include <arch/x86_64/xhci.h>
#include <kstring.h>
#include <malloc.h>
#include <mt.h>
#include <pm.h>
#include <printk.h>
#include <time.h>
#include <vm.h>

LIST_HEAD(g_ctrls);
LIST_HEAD(g_usb_devs);

static volatile u64 g_last_csc_time = 0;

struct xhci_trb xhci_enable_slot(struct xhci_dev *dev)
{
    return xhci_cmd(dev, 0, 0, TRB_TYPE(TRB_ENABLE_SLOT));
}

struct xhci_trb xhci_address_device(struct xhci_dev *dev, u32 slot, uptr in_ctx, u32 bsr)
{
    return xhci_cmd(dev, in_ctx, 0, TRB_TYPE(TRB_ADDRESS_DEVICE) | TRB_SLOT(slot) | (bsr << 9));
}

struct xhci_trb xhci_evaluate_context(struct xhci_dev *dev, u32 slot, uptr in_ctx)
{
    return xhci_cmd(dev, in_ctx, 0, TRB_TYPE(TRB_EVALUATE_CTX) | TRB_SLOT(slot));
}

struct xhci_trb xhci_disable_slot(struct xhci_dev *dev, u32 slot)
{
    return xhci_cmd(dev, 0, 0, TRB_TYPE(TRB_DISABLE_SLOT) | TRB_SLOT(slot));
}

void xhci_ud_detach(struct xhci_usb_dev *ud)
{
    xhci_disable_slot(ud->ctrl, ud->slot);
    if (ud->ctrl->dcba_arr[ud->slot]) {
        pm_free_frame(ud->ctrl->dcba_arr[ud->slot]);
        ud->ctrl->dcba_arr[ud->slot] = NULL;
    }
    for (int i = 0; i < 16; i++) {
        if (ud->trs[i].base) {
            pm_free_frame(PHYS(ud->trs[i].base));
            ud->trs[i].base = NULL;
        }
    }
}

uptr xhci_alloc_page(uptr *phys)
{
    uptr page = pm_alloc_zero_frame();
    if (phys)
        *phys = page;
    vm_set_flags(VIRT(page), 1, VM_PG_NC | VM_PG_WT | VM_PG_WR);
    return VIRT(page);
}

void xhci_ring_init(struct xhci_ring *ring)
{
    ring->base = ring->enq = xhci_alloc_page(NULL);
    ring->base[255].param = PHYS(ring->base);
    ring->base[255].control = TRB_TYPE(TRB_LINK) | TRB_LINK_TC;
    ring->pcs = 1;
}

static inline struct xhci_slot_ctx *xhci_get_slot_ctx(struct xhci_dev *dev, struct xhci_in_ctrl_ctx *in_ctx)
{
    return (uptr)in_ctx + dev->ctx_sz;
}

static inline struct xhci_ep_ctx *xhci_get_ep_ctx(struct xhci_dev *dev, struct xhci_in_ctrl_ctx *in_ctx, int ep_idx)
{
    return (uptr)in_ctx + dev->ctx_sz * (ep_idx + 2);
}

static const char *xhci_speed_to_str(u8 speed)
{
    switch (speed) {
    case PORTSC_SPEED_LS:
        return "low-speed";
    case PORTSC_SPEED_FS:
        return "full-speed";
    case PORTSC_SPEED_HS:
        return "high-speed";
    case PORTSC_SPEED_SS:
        return "super-speed";
    case PORTSC_SPEED_SSP:
        return "super-speed+";
    default:
        return "unknown speed";
    }
}

#define WAIT(x)                                                                                                        \
    ({                                                                                                                 \
        u64 __t0 = time_get_up_ns();                                                                                   \
        while (!(x) && time_get_up_ns() - __t0 < 1000000000)                                                           \
            asm volatile("hlt");                                                                                       \
        !(x);                                                                                                          \
    })

static void __xhci_read_string_desc(struct xhci_dev *dev, struct xhci_ring *tr, u8 slot, uptr phys_buf, u8 idx,
                                    u16 langid)
{
    xhci_write_trb(tr,
                   *(u64 *)&(struct xhci_setup_trb_param){
                       .bmRequestType = 0x80, .bRequest = 6, .wValue = 0x0300 | idx, .wIndex = langid, .wLength = 2},
                   TRB_TRANSFER_LEN(8), TRB_TYPE(TRB_SETUP) | TRB_SETUP_TRT(TRT_IN_DATA) | TRB_IDT);
    xhci_write_trb(tr, phys_buf, TRB_TRANSFER_LEN(2), TRB_TYPE(TRB_DATA) | TRB_CTRL_DIR(DIR_IN));
    xhci_write_trb(tr, 0, 0, TRB_TYPE(TRB_STATUS) | TRB_CTRL_DIR(DIR_OUT) | TRB_IOC);

    dev->xfer_evt = false;
    dev->dbs[slot] = 1;
    if (WAIT(dev->xfer_evt)) {
        printk("XHCI ERR: TIMEOUT EXCEEDED");
        return;
    }

    struct usb_string *str = VIRT(phys_buf);

    xhci_write_trb(
        tr,
        *(u64 *)&(struct xhci_setup_trb_param){
            .bmRequestType = 0x80, .bRequest = 6, .wValue = 0x0300 | idx, .wIndex = langid, .wLength = str->bLength},
        TRB_TRANSFER_LEN(8), TRB_TYPE(TRB_SETUP) | TRB_SETUP_TRT(TRT_IN_DATA) | TRB_IDT);
    xhci_write_trb(tr, phys_buf, TRB_TRANSFER_LEN(str->bLength), TRB_TYPE(TRB_DATA) | TRB_CTRL_DIR(DIR_IN));
    xhci_write_trb(tr, 0, 0, TRB_TYPE(TRB_STATUS) | TRB_CTRL_DIR(DIR_OUT) | TRB_IOC);

    memset(str, 0, str->bLength);

    dev->xfer_evt = false;
    dev->dbs[slot] = 1;
    if (WAIT(dev->xfer_evt)) {
        printk("XHCI ERR: TIMEOUT EXCEEDED");
        return;
    }
}

bool xhci_usb_dev_init(struct xhci_usb_dev *ud)
{
    // print("USB device init on port %hhu\n", ud->port);
    if (ud->slot) {
        xhci_ud_detach(ud);
    }

    volatile struct xhci_port *prt = &ud->ctrl->op->ports[ud->port];

    prt->portsc |= PORTSC_RESET;
    if (WAIT(prt->portsc & PORTSC_PRC)) {
        print("port reset failed: no response\n");
        return false;
    }
    prt->portsc &= ~PORTSC_PED;

    struct xhci_trb res = xhci_enable_slot(ud->ctrl);
    if (EV_TRB_CC(res.status) != CC_SUCCESS) {
        print("enable slot failed, status: %08x\n", res.status);
        return false;
    }
    ud->slot = res.control >> 24;

    xhci_ring_init(&ud->trs[0]);

    uptr in_ctx_phys;
    struct xhci_in_ctrl_ctx *in_ctx = xhci_alloc_page(&in_ctx_phys);
    in_ctx->add_ctx_flags = 0b11;

    struct xhci_slot_ctx *sl = xhci_get_slot_ctx(ud->ctrl, in_ctx);
    sl->root_port_num = ud->port + 1;
    sl->context_entries = 1;
    sl->speed = PORTSC_GET_SPEED(prt->portsc);

    struct xhci_ep_ctx *ep0 = xhci_get_ep_ctx(ud->ctrl, in_ctx, 0);
    ep0->ep_type = EP_CONTROL;
    ep0->cerr = 3;
    ep0->trdp = PHYS(ud->trs[0].base) | 1;
    switch (sl->speed) {
    case PORTSC_SPEED_LS:
        ep0->max_packet_sz = 8;
        break;
    case PORTSC_SPEED_FS:
    case PORTSC_SPEED_HS:
        ep0->max_packet_sz = 64;
        break;

    default:
        if (sl->speed >= PORTSC_SPEED_SS) {
            ep0->max_packet_sz = 512;
        } else {
            print("unknown speed: %hhu\n", sl->speed);
            pm_free_frame(in_ctx_phys);
            return false;
        }
        break;
    }

    uptr out_ctx = xhci_alloc_page(&ud->ctrl->dcba_arr[ud->slot]);
    res = xhci_address_device(ud->ctrl, ud->slot, in_ctx_phys, 1);
    if (EV_TRB_CC(res.status) != CC_SUCCESS) {
        print("address device (BSR = 1) failed, status: %08x\n", res.status);
        pm_free_frame(in_ctx_phys);
        return false;
    }

    uptr data_buf;
    volatile struct usb_descriptor *desc = xhci_alloc_page(&data_buf);

    xhci_write_trb(&ud->trs[0],
                   *(u64 *)&(struct xhci_setup_trb_param){
                       .bmRequestType = 0x80, .bRequest = 6, .wValue = 0x0100, .wIndex = 0, .wLength = 18},
                   TRB_TRANSFER_LEN(8), TRB_TYPE(TRB_SETUP) | TRB_SETUP_TRT(TRT_IN_DATA) | TRB_IDT);
    xhci_write_trb(&ud->trs[0], data_buf, TRB_TRANSFER_LEN(18), TRB_TYPE(TRB_DATA) | TRB_CTRL_DIR(DIR_IN));
    xhci_write_trb(&ud->trs[0], 0, 0, TRB_TYPE(TRB_STATUS) | TRB_CTRL_DIR(DIR_OUT) | TRB_IOC);

    ud->ctrl->xfer_evt = false;
    ud->ctrl->dbs[ud->slot] = 1;
    if (WAIT(ud->ctrl->xfer_evt)) {
        print("get descriptor failed: no response\n");
        pm_free_frame(in_ctx_phys);
        pm_free_frame(data_buf);
        return false;
    }

    memcpy(sl, out_ctx, ud->ctrl->ctx_sz * 2);
    ep0->trdp = PHYS(ud->trs[0].enq) | 1;
    if (sl->speed < PORTSC_SPEED_SS) {
        ep0->max_packet_sz = desc->bMaxPacketSize0;
    } else {
        ep0->max_packet_sz = 1;
        for (int i = 0; i < desc->bMaxPacketSize0; i++) {
            ep0->max_packet_sz *= 2;
        }
    }

    res = xhci_address_device(ud->ctrl, ud->slot, in_ctx_phys, 0);
    pm_free_frame(in_ctx_phys);
    if (EV_TRB_CC(res.status) != CC_SUCCESS) {
        print("address device (BSR = 0) failed, status: %08x\n", res.status);
        pm_free_frame(data_buf);
        return false;
    }

    uptr str_data = data_buf + sizeof(struct usb_descriptor);
    struct usb_string *str = VIRT(str_data);
    struct usb_string_zero *str0 = str;

    print("[%s] %04hx:%04hx ", xhci_speed_to_str(sl->speed), desc->idVendor, desc->idProduct);

    __xhci_read_string_desc(ud->ctrl, &ud->trs[0], ud->slot, str_data, 0, 0);
    u16 langid0 = 0;
    if (str0->bLength > 2) {
        langid0 = str0->wLANGID[0];
    }

    if (desc->iManufacturer) {
        __xhci_read_string_desc(ud->ctrl, &ud->trs[0], ud->slot, str_data, desc->iManufacturer, langid0);
        for (int i = 0; i < (str->bLength - 2) / 2; i++) {
            printk("%hc", str->bString[i]);
        }
        printk(" ");
    }

    if (desc->iProduct) {
        __xhci_read_string_desc(ud->ctrl, &ud->trs[0], ud->slot, str_data, desc->iProduct, langid0);
        for (int i = 0; i < (str->bLength - 2) / 2; i++) {
            printk("%hc", str->bString[i]);
        }
        printk(" ");
    }

    if (desc->iSerialNumber) {
        __xhci_read_string_desc(ud->ctrl, &ud->trs[0], ud->slot, str_data, desc->iSerialNumber, langid0);
        for (int i = 0; i < (str->bLength - 2) / 2; i++) {
            printk("%hc", str->bString[i]);
        }
        printk(" ");
    }

    printk("\n");
    pm_free_frame(data_buf);

    return true;
}

void xhci_irq(struct intr_frame *fr)
{
    struct xhci_dev *ctrl;
    list_for_each_entry(ctrl, &g_ctrls, lh)
    {
        if (ctrl->op->usbsts & (1 << 2)) {
            print("host system error detected\n");
        }

        if (ctrl->op->usbsts & (1 << 3)) {
            ctrl->op->usbsts |= (1 << 3);
            ctrl->rt->irs[0].iman |= 1;
        }

        u32 trb_ctrl = ctrl->evt_ring_deq->control;

        while ((trb_ctrl & 1) == ctrl->evt_ring_ccs) {
            switch (TRB_GET_TYPE(trb_ctrl)) {
            case TRB_PORT_STATUS_CHANGE_EVENT:
                u8 port_id = (ctrl->evt_ring_deq->param >> 24) & 0xff;
                volatile struct xhci_port *prt = &ctrl->op->ports[port_id - 1];
                if (prt->portsc & (1 << 17)) {
                    print("PSC Event [%zx]: Port ID: %hhu, Status: %s, PORTSC: %08x\n", ctrl->rt->irs[0].erdp, port_id,
                          prt->portsc & 1 ? "Connect" : "Disconnect", prt->portsc);
                    ctrl->csc_pending = true;
                    g_last_csc_time = time_get_up_ns();
                }
                break;

            case TRB_CMD_COMPLETION_EVENT:
                print("Command Completion Event [%zx] Cmd TRB: %zx, Status: %08x\n", ctrl->rt->irs[0].erdp,
                      ctrl->evt_ring_deq->param, ctrl->evt_ring_deq->status);
                ctrl->last_cc_evt = *ctrl->evt_ring_deq;
                break;

            case TRB_TRANSFER_EVENT:
                /*print("CTRL(%d) Transfer Event [%zx] Param: %zx, Status: %08x\n", i, ctrl->rt->irs[0].erdp,
                      ctrl->evt_ring_deq->param, ctrl->evt_ring_deq->status);*/
                ctrl->xfer_evt = true;
                break;

            default:
                print("unhandled event TRB [%zx]: %016zx %08x %08x\n", ctrl->rt->irs[0].erdp, ctrl->evt_ring_deq->param,
                      ctrl->evt_ring_deq->status, ctrl->evt_ring_deq->control);
                break;
            }

            if ((ctrl->rt->irs[0].erdp - ctrl->erst[0].rsba) / sizeof(struct xhci_trb) == ctrl->erst[0].rssz - 1) {
                ctrl->evt_ring_deq = VIRT(ctrl->erst[0].rsba);
                ctrl->evt_ring_ccs ^= 1;
                ctrl->rt->irs[0].erdp = ctrl->erst[0].rsba | (1 << 3);
            } else {
                ctrl->evt_ring_deq++;
                ctrl->rt->irs[0].erdp = PHYS(ctrl->evt_ring_deq) | (1 << 3);
            }
            trb_ctrl = ctrl->evt_ring_deq->control;
        }
    }
    *(volatile u32 *)VIRT(0xFEE000B0) = 0;
}

void xhci_init_dev(union pci_addr pa, struct pci_common_header *hdr)
{
    if (hdr->class_code != 0x0C || hdr->subclass != 0x03 || hdr->prog_if != 0x30) {
        return;
    }

    irq_set_handler(16, xhci_irq);

    usize bar_size;
    uptr bar = pci_read_memory_bar(pa, 0, &bar_size);
    uptr mmio_base = vm_map_mmio(bar, PG_IDX_UP(bar_size));

    print("at PCI %02hhx:%02hhx.%hhx, MMIO base: %016zx (BAR size: %zu KiB)\n", pa.bus, pa.dev, pa.func, bar,
          bar_size / 1024);

    u8 msix_offset = 0;
    if (hdr->status & (1 << 4)) {
        print("PCI capabilities: ");
        u8 cap_off = pci_readl(pa, 0x34) & 0xff;
        while (cap_off) {
            u32 dw = pci_readl(pa, cap_off);
            printk("0x%hhx ", dw & 0xff);
            if ((dw & 0xff) == 0x11) {
                msix_offset = cap_off;
            }
            cap_off = (dw >> 8) & 0xff;
        }
        printk("\n");
    }

    struct xhci_dev *dev = kzalloc(sizeof(struct xhci_dev));
    dev->cap = mmio_base;
    dev->op = mmio_base + dev->cap->caplength;
    dev->max_ports = dev->cap->hcsparams1 >> 24;
    dev->max_slots = dev->cap->hcsparams1 & 0xff;
    dev->ctx_sz = (dev->cap->hccparams1 & (1 << 2)) ? 64 : 32;

    uptr ext_cap = mmio_base + (dev->cap->hccparams1 >> 16) * 4;

    while (1) {
        u32 *caps = ext_cap;
        if ((caps[0] & 0xff) == 2) {
            u8 maj = caps[0] >> 24, min = (caps[0] >> 16) & 0xff, psic = caps[2] >> 28;
            u32 name = caps[1];
            print("%.4s%hhx.%hhx ports %hhu-%hhu, psic: %hhu\n", &name, maj, min, caps[2] & 0xff,
                  (caps[2] & 0xff) + ((caps[2] >> 8) & 0xff) - 1, psic);
            for (u8 i = 0; i < psic; i++) {
                u32 psi = caps[4 + i];
                print("psiv: %hhu, speed: %hu * 10 ^ %hhu bit/s, lp: %hhu\n", psi & 0b1111, psi >> 16,
                      (psi >> 4) & 0b11, (psi >> 14) & 0b11);
            }
        }

        if ((caps[0] >> 8) & 0xff) {
            ext_cap += ((caps[0] >> 8) & 0xff) * 4;
        } else {
            break;
        }
    }

    print("cap length: %hhu, max ports: %hhu, max slots: %hhu, context size: %hhu bytes\n", dev->cap->caplength,
          dev->max_ports, dev->max_slots, dev->ctx_sz);

    print("performing HC reset...");
    dev->op->usbcmd |= (1 << 1);
    while (dev->op->usbcmd & (1 << 1))
        ;
    while (dev->op->usbsts & (1 << 11))
        ;
    printk(" done\n");

    dev->op->config |= dev->max_slots;
    dev->dcba_arr = xhci_alloc_page(&dev->op->dcbaap);
    print("allocated device context base address array: %016zx\n", dev->op->dcbaap);

    xhci_ring_init(&dev->cmd_ring);
    dev->op->crcr = PHYS(dev->cmd_ring.base) | 1;
    print("allocated command ring: %016zx\n", PHYS(dev->cmd_ring.base));

    u32 msb = (dev->cap->hcsparams2 >> 27) | (((dev->cap->hcsparams2 >> 21) & 0x1f) << 5);
    if (msb) {
        if (msb > 512) {
            print("init error: contiguous page allocation not supported\n");
            return;
        }
        uptr *sba = VIRT(pm_alloc_zero_frame());
        for (int i = 0; i < msb; i++) {
            sba[i] = pm_alloc_zero_frame();
        }
        dev->dcba_arr[0] = PHYS(sba);
        print("allocated %u scratchpad buffers\n", msb);
    }

    dev->rt = mmio_base + dev->cap->rtsoff;
    print("runtime registers at %016zx\n", PHYS(dev->rt));

    dev->dbs = mmio_base + dev->cap->dboff;
    print("doorbells at %016zx\n", PHYS(dev->dbs));

    if (msix_offset) {
        pci_writel(pa, msix_offset, pci_readl(pa, msix_offset) | (1 << 31));
        u32 toff = pci_readl(pa, msix_offset + 4);
        uptr table_addr = pci_read_memory_bar(pa, toff & 0b111, NULL) + (toff & ~0b111);
        print("MSI-X table at %016zx\n", table_addr);

        volatile struct pci_msix_ent *tbl = vm_map_mmio(PALIGN_DOWN(table_addr), 1) | (table_addr & 0xfff);
        tbl[0].msg_addr = 0xFEE00000;
        tbl[0].msg_data = 48;
        tbl[0].vec_ctrl = 1;

        dev->msix = tbl;
    }

    dev->erst = VIRT(pm_alloc_zero_frame());
    dev->erst[0].rsba = pm_alloc_zero_frame();
    dev->erst[0].rssz = 256;
    print("allocated event ring segment table: %016zx\n", PHYS(dev->erst));

    dev->rt->irs[0].iman = (1 << 1);
    dev->rt->irs[0].imod = 4000;
    dev->rt->irs[0].erstsz = 1;
    dev->rt->irs[0].erdp = dev->erst[0].rsba;
    dev->rt->irs[0].erstba = PHYS(dev->erst);

    dev->evt_ring_deq = vm_map_mmio(dev->erst[0].rsba, 1);
    print("ERST[0] ring segment base: %016zx\n", dev->erst[0].rsba);
    dev->evt_ring_ccs = 1;

    list_add(&dev->lh, &g_ctrls);
    dev->op->usbcmd |= (1 << 0) | (1 << 2);
    print("switched to running state\n");
}

void xhci_write_trb(struct xhci_ring *ring, u64 param, u32 status, u32 control)
{
    ring->enq->param = param;
    ring->enq->status = status;
    (ring->enq++)->control = (control & ~1) | ring->pcs;
    if (TRB_GET_TYPE(ring->enq->control) == TRB_LINK) {
        if (ring->enq->control & TRB_LINK_TC) {
            ring->pcs ^= 1;
        }
        ring->enq = VIRT(ring->enq->param);
    }
}

struct xhci_trb xhci_cmd(struct xhci_dev *dev, u64 param, u32 status, u32 control)
{
    uptr enq = PHYS(dev->cmd_ring.enq);
    xhci_write_trb(&dev->cmd_ring, param, status, control);
    dev->dbs[0] = 0;
    if (WAIT(dev->last_cc_evt.param == enq)) {
        print("cmd failed: no response\n");
        return (struct xhci_trb){};
    }
    struct xhci_trb out = dev->last_cc_evt;
    dev->last_cc_evt.param = NULL;
    return out;
}

void xhci_csc_handler()
{
    while (1) {
        while (time_get_up_ns() - g_last_csc_time < 1000000000) {
            mt_switch_thread();
        }
        struct xhci_dev *d;
        list_for_each_entry(d, &g_ctrls, lh)
        {
            while (d->csc_pending) {
                d->csc_pending = false;
                for (int prt = 0; prt < d->max_ports; prt++) {
                    volatile struct xhci_port *p = &d->op->ports[prt];
                    if (p->portsc & PORTSC_CSC) {
                        struct xhci_usb_dev *ud = NULL, *cur;
                        list_for_each_entry(cur, &g_usb_devs, lh)
                        {
                            if (cur->ctrl == d && cur->port == prt) {
                                ud = cur;
                                break;
                            }
                        }
                        if (p->portsc & PORTSC_CONNECT) {
                            p->portsc &= ~PORTSC_PED;
                            if (!ud) {
                                ud = kzalloc(sizeof(struct xhci_usb_dev));
                                ud->ctrl = d;
                                ud->port = prt;
                                list_add(&ud->lh, &g_usb_devs);
                            }

                            bool init_ok;
                            do {
                                init_ok = xhci_usb_dev_init(ud);
                            } while (!init_ok && (p->portsc & PORTSC_CONNECT));

                            if (!init_ok) {
                                print("device init failed on port %hhu\n", ud->port + 1);
                                p->portsc &= ~PORTSC_PED;
                                xhci_ud_detach(ud);
                                list_del(&ud->lh);
                                kfree(ud);
                            }
                        } else {
                            p->portsc &= ~PORTSC_PED;
                            if (ud) {
                                print("detach on port %hhu (slot %hhu)\n", ud->port + 1, ud->slot);
                                xhci_ud_detach(ud);
                                list_del(&ud->lh);
                                kfree(ud);
                            }
                        }
                    }
                }
            }
        }
        mt_switch_thread();
    }
}

void xhci_init()
{
    pci_scan(xhci_init_dev);

    struct xhci_dev *ctrl;
    list_for_each_entry(ctrl, &g_ctrls, lh)
    {
        ctrl->msix[0].vec_ctrl = 0;
    }
    list_for_each_entry(ctrl, &g_ctrls, lh)
    {
        for (int prt = 0; prt < ctrl->max_ports; prt++) {
            if (ctrl->op->ports[prt].portsc & 1) {
                struct xhci_usb_dev *ud = kzalloc(sizeof(struct xhci_usb_dev));
                ud->ctrl = ctrl;
                ud->port = prt;
                list_add(&ud->lh, &g_usb_devs);
                bool init_ok;
                do {
                    init_ok = xhci_usb_dev_init(ud);
                } while (!init_ok && (ctrl->op->ports[prt].portsc & PORTSC_CONNECT));

                if (!init_ok) {
                    print("device init failed on port %hhu\n", ud->port + 1);
                    ctrl->op->ports[prt].portsc &= ~PORTSC_PED;
                    xhci_ud_detach(ud);
                    list_del(&ud->lh);
                    kfree(ud);
                }
            }
        }
    }
    struct thread *csc_td = mt_create_kthread(xhci_csc_handler, NULL);
    mt_start_thread(csc_td);
}