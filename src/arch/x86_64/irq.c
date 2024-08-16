#include <arch/x86_64/idt.h>
#include <arch/x86_64/irq.h>
#include <arch/x86_64/pic.h>
#include <types.h>
#include <printk.h>
#define PRINT_FMT(x) "irq: " x

#define IRQ_NUM 17

static uptr g_handlers[IRQ_NUM];

void irq_set_handler(u8 num, uptr hnd)
{
    if (num < IRQ_NUM) {
        g_handlers[num] = hnd;
    }
}

void irq_handler(struct intr_frame *fr)
{
    if (g_handlers[fr->int_no - 32]) {
        ((void (*)(struct intr_frame *))g_handlers[fr->int_no - 32])(fr);
    }
    if (fr->int_no < 48) {
        pic_eoi(fr->int_no - 32);
    }
}

void irq_init()
{
    extern uptr irq_stubs[];
    for (u8 i = 0; i < IRQ_NUM; i++) {
        idt_set_entry(32 + i, irq_stubs[i], 0x8E, 0x08);
    }
}