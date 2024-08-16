#include <arch/x86_64/asm.h>
#include <arch/x86_64/irq.h>
#include <arch/x86_64/pio.h>
#include <arch/x86_64/ps2dkbd.h>
#include <printk.h>

#define PRINT_FMT(x) "ps2dkbd: " x "\n"

#define KBD_RBUF_SIZE 128

#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_CMD 0x64

usize rbuf_wr_idx = 0, rbuf_rd_idx = 0;

static u8 kbd_rbuf[KBD_RBUF_SIZE];

static void ps2kbd_irq_handler(struct intr_frame *fr)
{
    while ((inb(PS2_STATUS) & 1) == 0) {
        cpu_pause();
    }
    u8 sc = inb(PS2_DATA);
    kbd_rbuf[rbuf_wr_idx++] = sc;
    rbuf_wr_idx %= KBD_RBUF_SIZE;
    printk("%hhX ", sc);
    if (sc == 0xb) {
        *(volatile u64 *)0 = 0;
    }
}

void ps2dkbd_init()
{
    inb(PS2_DATA);
    irq_set_handler(1, ps2kbd_irq_handler);
}

u8 ps2dkbd_read()
{
    while (rbuf_rd_idx == rbuf_wr_idx) {
        asm volatile("hlt");
    }
    u8 ret = kbd_rbuf[rbuf_rd_idx++];
    rbuf_rd_idx %= KBD_RBUF_SIZE;
    return ret;
}