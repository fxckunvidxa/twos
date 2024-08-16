#include <arch/x86_64/idt.h>
#include <arch/x86_64/isr.h>
#include <arch/x86_64/asm.h>
#include <vm.h>
#include <printk.h>

#define ISR_NUM 32

static uptr g_handlers[ISR_NUM];

void isr_set_handler(u8 num, uptr hnd)
{
    if (num < ISR_NUM) {
        g_handlers[num] = hnd;
    }
}

static void pf_handler(struct intr_frame *fr)
{
    uptr fa;
    asm volatile("mov %%cr2, %0" : "=r"(fa));

    const struct {
        usize prsnt : 1;
        usize write : 1;
        usize user : 1;
        usize rsvd : 1;
        usize iftch : 1;
    } *ec = &fr->err_code;

    if (!ec->prsnt && fa >= VM_PHYS_MAP_START && fa < VM_MR_KSTACK_START) {
        vm_map_page(vm_get_cr3(), PHYS(PALIGN_DOWN(fa)), PALIGN_DOWN(fa), VM_PG_WR);
        return;
    }

    printk("page fault: %016llx (pr=%zu wr=%zu us=%zu rs=%zu if=%zu)\nrip=%04llx:%016llx, rsp="
           "%04llx:%016llx",
           fa, ec->prsnt, ec->write, ec->user, ec->rsvd, ec->iftch, fr->cs, fr->rip, fr->ss, fr->rsp);
    panic("PF while in kernel mode");
}

void isr_handler(struct intr_frame *fr)
{
    if (fr->int_no < ISR_NUM && g_handlers[fr->int_no] != NULL) {
        void (*handler)(struct intr_frame *) = g_handlers[fr->int_no];
        handler(fr);
    } else {
        printk("\nunhandled exception %llu err: %llu rip: %04llX:%016llX, rsp: "
               "%04llX:%016llX",
               fr->int_no, fr->err_code, fr->cs, fr->rip, fr->ss, fr->rsp);
        panic("see above");
    }
}

void isr_init()
{
    extern uptr isr_stubs[];
    for (u8 i = 0; i < ISR_NUM; i++) {
        idt_set_entry(i, isr_stubs[i], 0x8E, 0x08);
    }
    isr_set_handler(14, pf_handler);
}
