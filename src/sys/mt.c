#include <arch/x86_64/rtc.h>
#include <list.h>
#include <malloc.h>
#include <printk.h>
#include <mt.h>
#include <time.h>
#include <vm.h>
#include <vt.h>

LIST_HEAD(g_td_queue);

static struct thread *g_current_td, *g_idle_td;
bool g_sched_enabled = false;

struct thread *mt_create_kthread(uptr ip, uptr data)
{
    struct thread *td = kzalloc(sizeof(struct thread));
    arch_init_kthread_ctx(td, ip, data);
    return td;
}

static struct thread *mt_get_next_td()
{
    if (list_empty(&g_td_queue)) {
        return g_idle_td;
    }
    struct thread *td = list_first_entry(&g_td_queue, struct thread, lh);
    list_del(&td->lh);
    return td;
}

void mt_switch_thread()
{
    if (!g_sched_enabled) {
        return;
    }

    asm volatile("cli");
    if (arch_save_ctx(&g_current_td->ctx)) {
        asm volatile("sti");
        return;
    }

    if (g_current_td != g_idle_td) {
        list_add_tail(&g_current_td->lh, &g_td_queue);
    }
    g_current_td = mt_get_next_td();
    arch_restore_ctx(&g_current_td->ctx);
}

void mt_start_thread(struct thread *td)
{
    list_add_tail(&td->lh, &g_td_queue);
}

void mt_init()
{
    g_idle_td = mt_create_kthread(arch_idle, NULL);
    g_current_td = kzalloc(sizeof(struct thread));
    g_sched_enabled = true;
}
