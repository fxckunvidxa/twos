#define PRINT_FMT(x) "tsc: " x

#include <arch/x86_64/asm.h>
#include <arch/x86_64/acpipmt.h>
#include <arch/x86_64/tsc.h>
#include <printk.h>

u64 tsc_get_freq_khz()
{
    if (!acpipmt_is_supported()) {
        return 3000000;
    }
    u64 s_tsc, e_tsc, tsc_khz;
    u32 s_tmr, e_tmr, mask = acpipmt_get_mask();
    print("calibrating TSC using ACPI PM timer\n");
    asm volatile("cli");
    s_tmr = e_tmr = acpipmt_read();
    s_tsc = read_tsc();
    while (((e_tmr - s_tmr) & mask) < 300000) {
        e_tmr = acpipmt_read();
    }
    e_tsc = read_tsc();
    asm volatile("sti");
    tsc_khz = (e_tsc - s_tsc) * PM_TMR_FREQ / ((e_tmr - s_tmr) & mask) / 1000;
    print("TSC frequency: %llu.%03llu MHz\n", tsc_khz / 1000, tsc_khz % 1000);
    return tsc_khz;
}
