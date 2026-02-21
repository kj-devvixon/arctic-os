#include "../include/kernel.h"
#include <stdarg.h>

int kstrlen(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

char *kstrcpy(char *dst, const char *src) {
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

char *kstrcat(char *dst, const char *src) {
    char *d = dst + kstrlen(dst);
    kstrcpy(d, src);
    return dst;
}

int kstrcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (u8)*a - (u8)*b;
}

int kstrncmp(const char *a, const char *b, int n) {
    while (n-- && *a && *a == *b) { a++; b++; }
    if (n < 0) return 0;
    return (u8)*a - (u8)*b;
}

void *kmemset(void *ptr, int val, size_t n) {
    u8 *p = ptr;
    while (n--) *p++ = (u8)val;
    return ptr;
}

void *kmemcpy(void *dst, const void *src, size_t n) {
    u8 *d = dst;
    const u8 *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void kitoa(i32 val, char *buf, int base) {
    char tmp[32];
    int i = 0, neg = 0;
    if (val == 0) { buf[0]='0'; buf[1]='\0'; return; }
    if (val < 0 && base == 10) { neg = 1; val = -val; }
    while (val > 0) {
        int r = val % base;
        tmp[i++] = r < 10 ? '0'+r : 'a'+(r-10);
        val /= base;
    }
    if (neg) tmp[i++] = '-';
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

void kutoa(u32 val, char *buf, int base) {
    char tmp[32];
    int i = 0;
    if (val == 0) { buf[0]='0'; buf[1]='\0'; return; }
    while (val > 0) {
        int r = val % base;
        tmp[i++] = r < 10 ? '0'+r : 'a'+(r-10);
        val /= base;
    }
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

int katoi(const char *s) {
    int r = 0, neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') r = r*10 + (*s++ - '0');
    return neg ? -r : r;
}

void ksprintf(char *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *out = buf;
    char tmp[32];
    while (*fmt) {
        if (*fmt != '%') { *out++ = *fmt++; continue; }
        fmt++;
        switch (*fmt) {
            case 'd': {
                i32 v = va_arg(args, i32);
                kitoa(v, tmp, 10);
                kstrcpy(out, tmp);
                out += kstrlen(tmp);
                break;
            }
            case 'u': {
                u32 v = va_arg(args, u32);
                kutoa(v, tmp, 10);
                kstrcpy(out, tmp);
                out += kstrlen(tmp);
                break;
            }
            case 'x': {
                u32 v = va_arg(args, u32);
                kutoa(v, tmp, 16);
                kstrcpy(out, tmp);
                out += kstrlen(tmp);
                break;
            }
            case 's': {
                const char *s = va_arg(args, const char*);
                kstrcpy(out, s);
                out += kstrlen(s);
                break;
            }
            case 'c': {
                *out++ = (char)va_arg(args, int);
                break;
            }
            case '0': {
                // %02d lub %02u style
                fmt++; // cyfra szerokości
                int width = *fmt - '0';
                fmt++; // typ: d lub u
                char type = *fmt;
                i32 v = va_arg(args, i32);
                if (type == 'u')
                    kutoa((u32)v, tmp, 10);
                else
                    kitoa(v, tmp, 10);
                int l = kstrlen(tmp);
                while (l < width) { *out++ = '0'; l++; }
                kstrcpy(out, tmp);
                out += kstrlen(tmp);
                break;
            }
            default: *out++ = *fmt; break;
        }
        fmt++;
    }
    *out = '\0';
    va_end(args);
}