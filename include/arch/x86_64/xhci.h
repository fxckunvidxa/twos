#pragma once
#include <list.h>
#include <types.h>

#define TRB_GET_TYPE(trb_control) (((trb_control) >> 10) & 0x3f)

#define TRB_TYPE(x) ((x) << 10)
#define TRB_SLOT(x) ((x) << 24)
#define TRB_CTRL_DIR(x) ((x) << 16)
#define DIR_IN 1
#define DIR_OUT 0
#define TRB_SETUP_TRT(x) ((x) << 16)
#define TRT_NO_DATA 0
#define TRT_OUT_DATA 2
#define TRT_IN_DATA 3
#define TRB_TRANSFER_LEN(x) (x)
#define TRB_INTR_TARGET(x) ((x) << 22)

#define TRB_SETUP 2
#define TRB_DATA 3
#define TRB_STATUS 4
#define TRB_LINK 6
#define TRB_ENABLE_SLOT 9
#define TRB_DISABLE_SLOT 10
#define TRB_ADDRESS_DEVICE 11
#define TRB_EVALUATE_CTX 13

#define TRB_TRANSFER_EVENT 32
#define TRB_CMD_COMPLETION_EVENT 33
#define TRB_PORT_STATUS_CHANGE_EVENT 34

#define TRB_LINK_TC (1 << 1)
#define TRB_IOC (1 << 5)
#define TRB_IDT (1 << 6)

#define PORTSC_CONNECT 1
#define PORTSC_PED (1 << 1)
#define PORTSC_RESET (1 << 4)
#define PORTSC_CSC (1 << 17)
#define PORTSC_PRC (1 << 21)

#define PORTSC_GET_SPEED(portsc) (((portsc) >> 10) & 0xf)
#define PORTSC_SPEED_FS 1
#define PORTSC_SPEED_LS 2
#define PORTSC_SPEED_HS 3
#define PORTSC_SPEED_SS 4
#define PORTSC_SPEED_SSP 5

#define EP_ISOCH_OUT 1
#define EP_BULK_OUT 2
#define EP_INTERRUPT_OUT 3
#define EP_CONTROL 4
#define EP_ISOCH_IN 5
#define EP_BULK_IN 6
#define EP_INTERRUPT_IN 7

#define EV_TRB_CC(x) ((x) >> 24)
#define CC_SUCCESS 1

struct xhci_cap {
    u8 caplength;
    u8 __rsvd;
    u16 hciversion;
    u32 hcsparams1;
    u32 hcsparams2;
    u32 hcsparams3;
    u32 hccparams1;
    u32 dboff;
    u32 rtsoff;
    u32 hccparams2;
} __attribute__((packed));

struct xhci_port {
    u32 portsc;
    u32 portpmsc;
    u32 portli;
    u32 porthlpmc;
} __attribute__((packed));

struct xhci_op {
    u32 usbcmd;
    u32 usbsts;
    u32 pagesize;
    u64 __rsvd1;
    u32 dnctrl;
    u64 crcr;
    u8 __rsvd2[16];
    u64 dcbaap;
    u32 config;
    u8 __rsvd3[964];
    struct xhci_port ports[1];
} __attribute__((packed));

struct xhci_trb {
    u64 param;
    u32 status;
    u32 control;
} __attribute__((packed));

struct xhci_irs {
    u32 iman;
    u32 imod;
    u32 erstsz;
    u32 __rsvd;
    uptr erstba;
    uptr erdp;
} __attribute__((packed));

struct xhci_rt {
    u32 mfindex;
    u8 __rsvd[28];
    struct xhci_irs irs[1024];
} __attribute__((packed));

struct xhci_erst_ent {
    uptr rsba;
    u32 rssz;
    u32 __rsvd;
} __attribute__((packed));

struct xhci_slot_ctx {
    u32 route_string : 20;
    u8 speed : 4;
    u8 __rsvd : 1;
    u8 mtt : 1;
    u8 hub : 1;
    u8 context_entries : 5;

    u16 max_exit_latency;
    u8 root_port_num;
    u8 num_ports;

    u8 tt_hub_slot_id;
    u8 tt_port_num;
    u8 ttt : 2;
    u8 __rsvd2 : 4;
    u16 intr_target : 10;

    u8 usb_dev_addr;
    u32 __rsvd3 : 19;
    u8 slot_state : 5;

    u32 __rsvd4[4];
} __attribute__((packed));

struct xhci_ep_ctx {
    u8 ep_state : 3;
    u8 __rsvd : 5;
    u8 mult : 2;
    u8 maxpstreams : 5;
    u8 lsa : 1;
    u8 interval;
    u8 max_esit_payload_hi;

    u8 __rsvd2 : 1;
    u8 cerr : 2;
    u8 ep_type : 3;
    u8 __rsvd3 : 1;
    u8 hid : 1;
    u8 max_burst_sz;
    u16 max_packet_sz;

    uptr trdp;

    u16 avg_trb_len;
    u16 max_esit_payload_lo;

    u32 __rsvd4[3];
} __attribute__((packed));

struct xhci_in_ctrl_ctx {
    u32 drop_ctx_flags;
    u32 add_ctx_flags;
    u32 __rsvd[5];
    u8 config_value;
    u8 iface_number;
    u8 alt_setting;
    u8 __rsvd2;
} __attribute__((packed));

struct usb_descriptor {
    u8 bLength;
    u8 bDescriptorType;
    u16 bcdUSB;
    u8 bDeviceClass;
    u8 bDeviceSubClass;
    u8 bDeviceProtocol;
    u8 bMaxPacketSize0;
    u16 idVendor;
    u16 idProduct;
    u16 bcdDevice;
    u8 iManufacturer;
    u8 iProduct;
    u8 iSerialNumber;
    u8 bNumConfigurations;
} __attribute__((packed));

struct usb_string {
    u8 bLength;
    u8 bDescriptorType;
    u16 bString[1];
} __attribute__((packed));

struct usb_string_zero {
    u8 bLength;
    u8 bDescriptorType;
    u16 wLANGID[1];
} __attribute__((packed));

struct xhci_setup_trb_param {
    u8 bmRequestType;
    u8 bRequest;
    u16 wValue;
    u16 wIndex;
    u16 wLength;
} __attribute__((packed));

struct xhci_ring {
    volatile struct xhci_trb *base, *enq;
    u8 pcs;
};

struct xhci_usb_dev {
    struct list_head lh;

    struct xhci_dev *ctrl;
    struct xhci_ring trs[16];
    u8 slot, port;
};

struct xhci_dev {
    struct list_head lh;

    volatile struct xhci_cap *cap;
    volatile struct xhci_op *op;
    volatile struct xhci_rt *rt;
    volatile struct xhci_erst_ent *erst;
    volatile struct pci_msix_ent *msix;
    volatile u32 *dbs;
    volatile uptr *dcba_arr;
    volatile struct xhci_trb *evt_ring_deq;
    volatile struct xhci_trb last_cc_evt;
    struct xhci_ring cmd_ring;
    u8 max_ports, max_slots, evt_ring_ccs;
    usize ctx_sz;
    volatile bool csc_pending, xfer_evt;
};

void xhci_init();
void xhci_write_trb(struct xhci_ring *ring, u64 param, u32 status, u32 control);
struct xhci_trb xhci_cmd(struct xhci_dev *dev, u64 param, u32 status, u32 control);

struct xhci_trb xhci_enable_slot(struct xhci_dev *dev);
struct xhci_trb xhci_disable_slot(struct xhci_dev *dev, u32 slot);
struct xhci_trb xhci_evaluate_context(struct xhci_dev *dev, u32 slot, uptr in_ctx);
struct xhci_trb xhci_address_device(struct xhci_dev *dev, u32 slot, uptr in_ctx, u32 bsr);