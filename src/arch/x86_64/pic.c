#include <arch/x86_64/pic.h>
#include <arch/x86_64/pio.h>
#include <types.h>

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_CMD PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_CMD PIC2
#define PIC2_DATA (PIC2 + 1)

#define PIC1_OFF 32
#define PIC2_OFF 40

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

#define PIC_EOI 0x20

void pic_init()
{
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    pio_wait();
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    pio_wait();

    outb(PIC1_DATA, PIC1_OFF);
    pio_wait();
    outb(PIC2_DATA, PIC2_OFF);
    pio_wait();

    outb(PIC1_DATA, 4);
    pio_wait();
    outb(PIC2_DATA, 2);
    pio_wait();

    outb(PIC1_DATA, ICW4_8086);
    pio_wait();
    outb(PIC2_DATA, ICW4_8086);
    pio_wait();

    outb(PIC1_DATA, 0);
    outb(PIC2_DATA, 0);
}

void pic_eoi(u8 irq)
{
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);
}