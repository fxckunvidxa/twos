#pragma once

typedef unsigned char u8, uint8_t;
typedef char i8, int8_t;
typedef unsigned short u16, uint16_t;
typedef short i16, int16_t;
typedef unsigned int u32, uint32_t;
typedef int i32, int32_t;
typedef unsigned long long u64, usize, uint64_t, size_t, uptr, uintptr_t;
typedef long long i64, isize, int64_t, ssize_t;
typedef _Bool bool;

#define true 1
#define false 0
#define NULL ((void *)0)
#define offsetof(st, m) ((usize)(&((st *)0)->m))
