#include <arch/x86_64/asm.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/irq.h>
#include <arch/x86_64/pic.h>
#include <arch/x86_64/pio.h>
#include <arch/x86_64/pit.h>
#include <arch/x86_64/rtc.h>
#include <vt.h>
#include <printk.h>
#include <types.h>
#include <clock.h>

#define PRINT_FMT(x) "pit: " x

static u64 g_pit_ticks = 0;

#define PIT_C0 0x40
#define PIT_C1 0x41
#define PIT_C2 0x42
#define PIT_CMD 0x43
#define PIT_FREQ 1193182UL
#define PIT_MODE 0x34
#define PIT_IRQ 0

u64 pit_read_ticks()
{
    return g_pit_ticks;
}

static void pit_irq(struct intr_frame *fr)
{
    g_pit_ticks++;
    asm volatile("sti");
    pic_eoi(PIT_IRQ);
    mt_switch_thread();
}

static void pit_set_freq(u32 hz)
{
    print("frequency set to %lu Hz\n", hz);
    u16 divr = PIT_FREQ / hz;
    outb(PIT_CMD, PIT_MODE);
    outb(PIT_C0, divr & 0xff);
    outb(PIT_C0, divr >> 8);
}

void pit_init()
{
    irq_set_handler(PIT_IRQ, pit_irq);
    pit_set_freq(100);
}