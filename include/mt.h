#pragma once
#include <arch/x86_64/ctx.h>
#include <list.h>
#include <types.h>

#define KSTACK_PAGE_COUNT 4

struct thread {
    struct list_head lh;
    struct arch_kern_ctx ctx;
    uptr kstack;
};

void arch_init_kthread_ctx(struct thread *td, uptr ip, uptr data);
void arch_idle();
void mt_init();
void mt_start_thread(struct thread *td);
struct thread *mt_create_kthread(uptr ip, uptr data);
void mt_switch_thread();