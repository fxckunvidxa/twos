#pragma once
#include <types.h>

extern void *memset(void *, int, usize);
extern void *memcpy(void *, const void *, usize);
extern void *memmove(void *, const void *, usize);
int memcmp(const void *s1, const void *s2, usize n);
void *memchr(const void *s, int c, usize n);
usize strlen(const char *s);
char *strdup(const char *s);
int strcmp(const char *s1, const char *s2);