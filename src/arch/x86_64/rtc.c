#include <arch/x86_64/acpi.h>
#include <arch/x86_64/pio.h>
#include <arch/x86_64/rtc.h>

#define CMOS_REG 0x70
#define CMOS_DATA 0x71

#define CMOS_YEAR_OFFS 2000

#define RTC_SECONDS 0
#define RTC_MINUTES 2
#define RTC_HOURS 4
#define RTC_DAY_OF_MONTH 7
#define RTC_MONTH 8
#define RTC_YEAR 9
#define RTC_REG_A 10
#define RTC_REG_B 11

#define RTC_UIP 0x80
#define RTC_DM_BINARY 0x04
#define RTC_24H 0x02

#define RTC_BCD2BIN(x) (((x) & 0x0f) + ((x) >> 4) * 10)

static inline u8 cmos_read(u8 reg)
{
    outb(CMOS_REG, reg);
    return inb(CMOS_DATA);
}

void rtc_read(struct rtc_time *tp)
{
    if (!tp) {
        return;
    }

    u16 year;
    u8 century = acpi_get_fadt()->CENTURY, mon, day, hour, min, sec, status;

    while (cmos_read(RTC_REG_A) & RTC_UIP) {
        asm volatile("pause");
    }

    sec = cmos_read(RTC_SECONDS);
    min = cmos_read(RTC_MINUTES);
    hour = cmos_read(RTC_HOURS);
    day = cmos_read(RTC_DAY_OF_MONTH);
    mon = cmos_read(RTC_MONTH);
    year = cmos_read(RTC_YEAR);

    if (century) {
        century = cmos_read(century);
    }

    status = cmos_read(RTC_REG_B);

    if (!(status & RTC_DM_BINARY)) {
        sec = RTC_BCD2BIN(sec);
        min = RTC_BCD2BIN(min);
        hour = RTC_BCD2BIN(hour & 0x7f) | (hour & 0x80);
        day = RTC_BCD2BIN(day);
        mon = RTC_BCD2BIN(mon);
        year = RTC_BCD2BIN(year);
        if (century) {
            century = RTC_BCD2BIN(century);
        }
    }

    if (!(status & RTC_24H) && (hour & 0x80)) {
        hour = ((hour & 0x7f) + 12) % 24;
    }

    if (century) {
        year += century * 100;
    } else {
        year += CMOS_YEAR_OFFS;
    }

    tp->year = year;
    tp->mon = mon;
    tp->day = day;
    tp->hour = hour;
    tp->min = min;
    tp->sec = sec;
}
