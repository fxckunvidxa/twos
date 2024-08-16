#pragma once
#include <types.h>

struct clock_info {
    char name[16];
    u64 (*read)();
    u64 mask;
    u64 frequency;
};

struct clock_info *clock_get_by_id(int id);
bool clock_register(const char *name, u64 freq, u64 mask, u64 (*rd)());
struct clock_info *clock_get(const char *name);