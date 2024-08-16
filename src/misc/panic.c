#include <panic.h>
#include <printk.h>
#include <time.h>
#include <arch/x86_64/asm.h>

void __attribute__((noreturn)) panic(const char *msg)
{
    printk("\n!!! panic: %s\n", msg);
    struct timespec t;
    time_get_uptime(&t);
    for (int i = 30; i > 0; i--) {
        printk("\rReboot in %d s...", i);
        time_t cur = t.tv_sec;
        while (t.tv_sec == cur) {
            time_get_uptime(&t);
            cpu_pause();
        }
    }

    arch_reboot();
}