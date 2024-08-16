#pragma once
#include <types.h>

struct ehci_cap {
    u8 caplength;
    u8 __rsvd;
    u16 hciversion;
    u32 hcsparams;
    u32 hccparams;
    u64 hcsp_portroute;
} __attribute__((packed));

struct ehci_op {
    u32 usbcmd;
    u32 usbsts;
    u32 usbintr;
    u32 frindex;
    u32 ctrldssegment;
    u32 periodiclistbase;
    u32 asynclistaddr;
    u8 __rsvd[36];
    u32 configflag;
    u32 portsc[1];
} __attribute__((packed));

struct ehci_dev {
    
};

void ehci_init();