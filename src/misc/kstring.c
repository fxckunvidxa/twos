#include <kstring.h>
#include <malloc.h>

int memcmp(const void *s1, const void *s2, usize n)
{
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        ++p1;
        ++p2;
    }
    return 0;
}

void *memchr(const void *s, int c, usize n)
{
    const unsigned char *p = (const unsigned char *)s;
    while (n--) {
        if (*p == (unsigned char)c) {
            return (void *)p;
        }
        ++p;
    }
    return NULL;
}

usize strlen(const char *s)
{
    usize r = 0;
    while (s[r]) {
        r++;
    }
    return r;
}

char *strdup(const char *s)
{
    if (!s)
        return NULL;
    
    usize l = strlen(s);
    char *o = kmalloc(l + 1);

    if (!o)
        return NULL;

    return memcpy(o, s, l + 1);
}

int strcmp(const char *s1, const char *s2)
{
    while ((*s1) && (*s1 == *s2))
    {
        ++s1;
        ++s2;
    }

    return (*(unsigned char *)s1 - *(unsigned char *)s2);
}