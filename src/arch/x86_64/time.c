#include <arch/x86_64/asm.h>
#include <arch/x86_64/acpipmt.h>
#include <arch/x86_64/rtc.h>
#include <arch/x86_64/tsc.h>
#include <printk.h>
#include <time.h>
#include <clock.h>

static u64 g_initial_tsc;
static u64 g_tsc_khz;
static time_t g_initial_time;

#define LEAP_DAYS(from, to) ((to) / 4 + (to) / 400 - (to) / 100 - (from) / 4 - (from) / 400 + (from) / 100)

time_t rtc_to_unix(const struct rtc_time *tm)
{
    static const int mdays[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int year_days = mdays[tm->mon - 1] + tm->day - 1;
    if (tm->mon >= 3 && (tm->year % 4 == 0 && (tm->year % 100 != 0 || tm->year % 400 == 0))) {
        year_days++;
    }
    time_t days = (tm->year - 1970) * 365 + LEAP_DAYS(1970, tm->year) + year_days;
    return days * 86400 + tm->hour * 3600 + tm->min * 60 + tm->sec;
}

void time_init()
{
    g_initial_tsc = read_tsc();
    struct rtc_time tm;
    rtc_read(&tm);
    g_initial_time = rtc_to_unix(&tm);
    print("boot time: %hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu\n", tm.year, tm.mon, tm.day, tm.hour, tm.min, tm.sec);
    acpipmt_init();
    g_tsc_khz = tsc_get_freq_khz();
}

u64 time_get_up_ns()
{
    return (read_tsc() - g_initial_tsc) * 1000000 / g_tsc_khz;
}

void time_get_uptime(struct timespec *tp)
{
    long nsec = time_get_up_ns();
    tp->tv_sec = nsec / 1000000000;
    tp->tv_nsec = nsec % 1000000000;
}

void time_get_realtime(struct timespec *tp)
{
    time_get_uptime(tp);
    tp->tv_sec += g_initial_time;
}