#pragma once
#include <types.h>

struct rtc_time {
    u16 year;
    u8 mon, day, hour, min, sec;
};

void rtc_read(struct rtc_time *);