#include <arch/x86_64/gdt.h>

static struct gdtr gdtr;
static struct gdt_ent gdt[] = {{0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00},
                               {0x0000, 0x0000, 0x00, 0x9A, 0x20, 0x00},
                               {0x0000, 0x0000, 0x00, 0x92, 0x00, 0x00},
                               {0x0000, 0x0000, 0x00, 0xFA, 0x20, 0x00},
                               {0x0000, 0x0000, 0x00, 0xF2, 0x00, 0x00}};

void gdt_init()
{
    gdtr.base = gdt;
    gdtr.limit = sizeof(gdt) - 1;
    asm volatile("lgdt %0\n"
                 "mov $0x10, %%rax\n"
                 "mov %%ax, %%ds\n"
                 "mov %%ax, %%es\n"
                 "mov %%ax, %%ss\n"
                 "mov %%ax, %%fs\n"
                 "mov %%ax, %%gs\n"
                 "push %%rax\n"
                 "push %%rsp\n"
                 "pushfq\n"
                 "pushq $0x08\n"
                 "pushq $1f\n"
                 "iretq\n"
                 "1:\n"
                 "add $8, %%rsp" ::"m"(gdtr)
                 : "rax");
}