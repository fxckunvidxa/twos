#include <kctype.h>

int isalnum(int c)
{
    return (isdigit(c) || isalpha(c));
}

int isalpha(int c)
{
    return islower(c) || isupper(c);
}

int isblank(int c)
{
    return c == ' ' || c == '\t';
}

int iscntrl(int c)
{
    return c >= 0 && c < 32;
}

int isdigit(int c)
{
    return c >= '0' && c <= '9';
}

int isgraph(int c)
{
    return c >= '!' && c <= '~';
}

int islower(int c)
{
    return c >= 'a' && c <= 'z';
}

int isprint(int c)
{
    return c >= ' ' && c <= '~';
}

int ispunct(int c)
{
    return isgraph(c) && !isalnum(c);
}

int isspace(int c)
{
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

int isupper(int c)
{
    return c >= 'A' && c <= 'Z';
}

int isxdigit(int c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int tolower(int c)
{
    if (c >= 'A' && c <= 'Z') {
        return 'a' + (c - 'A');
    }
    return c;
}

int toupper(int c)
{
    if (c >= 'a' && c <= 'z') {
        return 'A' + (c - 'a');
    }
    return c;
}