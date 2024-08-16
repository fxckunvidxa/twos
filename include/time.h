#pragma once
#include <types.h>

typedef i64 time_t;

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

void time_get_realtime(struct timespec *tp);
void time_get_uptime(struct timespec *tp);
u64 time_get_up_ns();