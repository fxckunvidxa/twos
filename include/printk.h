#pragma once
#include <types.h>

int printk(const char *format, ...);
int snprintf(char *str, usize size, const char *format, ...);

#ifndef PRINT_FMT
#define PRINT_FMT(fmt) fmt
#endif

#define print(fmt, ...) printk(PRINT_FMT(fmt), ##__VA_ARGS__)