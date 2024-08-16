#include <arch/x86_64/idt.h>

static struct idt_ent idt[256] __attribute__((aligned(0x10)));
static struct idtr idtr;

void idt_set_entry(u8 num, uptr hnd, u8 flags, u16 cs)
{
    struct idt_ent *ent = &idt[num];

    ent->base_low = hnd & 0xffff;
    ent->base_mid = (hnd >> 16) & 0xffff;
    ent->base_high = hnd >> 32;
    ent->cs = cs;
    ent->ist = 0;
    ent->attr = flags;
    ent->zero = 0;
}

void idt_init()
{
    idtr.base = idt;
    idtr.limit = sizeof(idt) - 1;
    asm volatile("lidt %0" ::"m"(idtr));
}