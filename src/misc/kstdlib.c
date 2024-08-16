#include <kstdlib.h>
#include <kstring.h>
#include <kctype.h>
#include <klimits.h>

static const char _digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

static const char *_strtox_prelim(const char *p, char *sign, int *base)
{
    while (isspace(*p)) {
        ++p;
    }
    if (*p != '+' && *p != '-') {
        *sign = '+';
    } else {
        *sign = *(p++);
    }
    if (*p == '0') {
        ++p;
        if ((*base == 0 || *base == 16) && (*p == 'x' || *p == 'X')) {
            *base = 16;
            ++p;
            if (memchr(_digits, tolower(*p), *base) == NULL) {
                p -= 2;
            }
        } else if (*base == 0) {
            *base = 8;
            --p;
        } else {
            --p;
        }
    } else if (!*base) {
        *base = 10;
    }

    return ((*base >= 2) && (*base <= 36)) ? p : NULL;
}

static long long unsigned int _strtox_main(const char **p, unsigned int base, long long unsigned int error,
                                           long long unsigned int limval, int limdigit, char *sign)
{
    long long unsigned int rc = 0;
    int digit = -1;
    const char *x;

    while ((x = (const char *)memchr(_digits, tolower(**p), base)) != NULL) {
        digit = x - _digits;

        if ((rc < limval) || ((rc == limval) && (digit <= limdigit))) {
            rc = rc * base + (unsigned)digit;
            ++(*p);
        } else {
            while (memchr(_digits, tolower(**p), base) != NULL) {
                ++(*p);
            }
            *sign = '+';
            return error;
        }
    }

    if (digit == -1) {
        *p = NULL;
        return 0;
    }

    return rc;
}

long int strtol(const char *s, char **endptr, int base)
{
    long int rc;
    char sign = '+';
    const char *p = _strtox_prelim(s, &sign, &base);

    if (base < 2 || base > 36) {
        return 0;
    }

    if (sign == '+') {
        rc = (long int)_strtox_main(&p, (unsigned)base, (long long unsigned int)LONG_MAX,
                                    (long long unsigned int)(LONG_MAX / base), (int)(LONG_MAX % base), &sign);
    } else {
        rc = (long int)_strtox_main(&p, (unsigned)base, (long long unsigned int)LONG_MIN,
                                    (long long unsigned int)(LONG_MIN / -base), (int)(-(LONG_MIN % base)), &sign);
    }

    if (endptr != NULL) {
        *endptr = (p != NULL) ? (char *)p : (char *)s;
    }

    return (sign == '+') ? rc : -rc;
}

long long int strtoll(const char *s, char **endptr, int base)
{
    long long int rc;
    char sign = '+';
    const char *p = _strtox_prelim(s, &sign, &base);

    if (base < 2 || base > 36) {
        return 0;
    }

    if (sign == '+') {
        rc = (long long int)_strtox_main(&p, (unsigned)base, (long long unsigned int)LLONG_MAX,
                                         (long long unsigned int)(LLONG_MAX / base), (int)(LLONG_MAX % base), &sign);
    } else {
        rc =
            (long long int)_strtox_main(&p, (unsigned)base, (long long unsigned int)LLONG_MIN,
                                        (long long unsigned int)(LLONG_MIN / -base), (int)(-(LLONG_MIN % base)), &sign);
    }

    if (endptr != NULL) {
        *endptr = (p != NULL) ? (char *)p : (char *)s;
    }

    return (sign == '+') ? rc : -rc;
}

unsigned long int strtoul(const char *s, char **endptr, int base)
{
    unsigned long int rc;
    char sign = '+';
    const char *p = _strtox_prelim(s, &sign, &base);

    if (base < 2 || base > 36) {
        return 0;
    }

    rc = (unsigned long int)_strtox_main(&p, (unsigned)base, (long long unsigned int)ULONG_MAX,
                                         (long long unsigned int)(ULONG_MAX / base), (int)(ULONG_MAX % base), &sign);

    if (endptr != NULL) {
        *endptr = (p != NULL) ? (char *)p : (char *)s;
    }

    return (sign == '+') ? rc : -rc;
}

unsigned long long int strtoull(const char *s, char **endptr, int base)
{
    unsigned long long int rc;
    char sign = '+';
    const char *p = _strtox_prelim(s, &sign, &base);

    if (base < 2 || base > 36) {
        return 0;
    }

    rc = _strtox_main(&p, (unsigned)base, (long long unsigned int)ULLONG_MAX,
                      (long long unsigned int)(ULLONG_MAX / base), (int)(ULLONG_MAX % base), &sign);

    if (endptr != NULL) {
        *endptr = (p != NULL) ? (char *)p : (char *)s;
    }

    return (sign == '+') ? rc : -rc;
}