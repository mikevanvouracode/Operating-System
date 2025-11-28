#include "header/stdlib/string.h"

void *memcpy(void *dest, const void *src, size_t n)
{
    char *d = dest;
    const char *s = src;
    for (size_t i = 0; i < n; i++)
        d[i] = s[i];
    return dest;
}

void *memset(void *s, int c, size_t n)
{
    unsigned char *p = s;
    for (size_t i = 0; i < n; i++)
        p[i] = (unsigned char)c;
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = s1, *p2 = s2;
    for (size_t i = 0; i < n; i++)
    {
        if (p1[i] != p2[i])
            return p1[i] < p2[i] ? -1 : 1;
    }
    return 0;
}

size_t strlen(const char *s)
{
    size_t len = 0;
    while (s[len])
    {
        len++;
    }
    return len;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *strcpy(char *dest, const char *src)
{
    char *saved = dest;
    while ((*dest++ = *src++))
        ;
    return saved;
}

static void reverse(char s[])
{
    int i, j;
    char c;
    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void itoa(int n, char s[])
{
    int i, sign;
    if ((sign = n) < 0)
        n = -n;
    i = 0;
    do
    {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

char *strcat(char *dest, const char *src)
{
    char *saved = dest;

    while (*dest)
    {
        dest++;
    }

    while ((*dest++ = *src++))
    {
        ;
    }

    return saved;
}