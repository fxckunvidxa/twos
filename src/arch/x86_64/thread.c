#include <mt.h>
#include <vm.h>

void arch_idle()
{
    while (1) {
        asm volatile("hlt");
    }
}

extern void arch_kthread_init();

void arch_init_kthread_ctx(struct thread *td, uptr ip, uptr data)
{
    td->ctx.ip = arch_kthread_init;
    td->kstack = vm_mmap(VM_MR_KSTACK, NULL, NULL, KSTACK_PAGE_COUNT);
    td->ctx.sp = td->kstack + KSTACK_PAGE_COUNT * PAGE_SIZE - 16;
    *(volatile u64 *)td->ctx.sp = data;
    *(volatile u64 *)(td->ctx.sp + 8) = ip;
}