#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

typedef enum {
    STATUS_OK,            // ok
    STATUS_ERR_NULL,      // NULL pointer
    STATUS_ERR_FORMAT,    // unknown / broken / unsupported specifier
    STATUS_ERR_RANGE,     // roman form 1 to 3999, zeckendorf only > 0
    STATUS_ERR_DIGIT,     // empty string or invalid dig
    STATUS_ERR_OVERFLOW,  // number does not fit in int / one specifier is too long
    STATUS_ERR_IO,        // fwrite failed
    STATUS_ERR_ALLOC    // malloc failed
} status_t;


int valid_base(int base){
    return base >= 2 && base <= 36;
}


int digit_val(char c, int upper){
    if (c >= '0' && c <= '9') return c - '0';
    if (upper && c >= 'A' && c <= 'Z') return c - 'A' + 10;
    if (!upper && c >= 'a' && c <= 'z') return c - 'a' + 10;
    return -1;
}


// %Ro - roman letters
status_t roman_str(int n, char* out){
    if (n < 1 || n > 3999) return STATUS_ERR_RANGE;

    static const int vals[13] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
    static const char* syms[13] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};

    out[0] = '\0'; // (out >= 16 chars)
    for (int i = 0; i < 13; i++){
        while (n >= vals[i]){
            strcat(out, syms[i]);
            n -= vals[i];
        }
    }
    return STATUS_OK;
}


// %Zr (out should be>= 64 chars)
status_t zeckendorf_str(unsigned int n, char* out){
    if (n == 0) return STATUS_ERR_RANGE;

    // fibonacci from 1 to n
    unsigned long long fib[48], a = 1, b = 2;
    int k = 0;
    while (a <= n){
        fib[k++] = a;
        unsigned long long t = a + b;
        a = b;
        b = t;
    }

    // from biggest to smallest f_numberrrr
    for (int i = k - 1; i >= 0; i--){
        out[i] = (n >= fib[i]) ? '1' : '0';
        if (n >= fib[i]) n -= (unsigned int)fib[i];
    }
    out[k] = '1';   // end (because none of the chosen nums aer placed together)
    out[k + 1] = '\0';
    return STATUS_OK;
}


// %Cv / %CV - int to base upper lower leters itd
status_t int_to_base(int n, int base, int upper, char* out){
    if (!valid_base(base)) base = 10;
    long long v = n;

    char tmp[40];
    int k = 0;
    if (v < 0) v = -v;
    do {
        int d = (int)(v % base);
        tmp[k++] = (char)(d < 10 ? '0' + d : (upper ? 'A' : 'a') + d - 10);
        v /= base;
    } while (v > 0);

    int p = 0;
    if (n < 0) out[p++] = '-';
    while (k > 0) out[p++] = tmp[--k]; // reverse
    out[p] = '\0';
    return STATUS_OK;
}


// %to / %TO
status_t base_to_int(const char* s, int base, int upper, int* res){
    if (s == NULL || res == NULL) return STATUS_ERR_NULL;
    if (!valid_base(base)) base = 10;

    int neg = (s[0] == '-');
    if (s[neg] == '\0') return STATUS_ERR_DIGIT;

    unsigned long long limit = neg ? (unsigned long long)INT_MAX + 1 : (unsigned long long)INT_MAX;
    unsigned long long v = 0;

    for (const char* c = s + neg; *c != '\0'; c++){
        int d = digit_val(*c, upper);
        if (d < 0 || d >= base) return STATUS_ERR_DIGIT;
        if (v > (limit - (unsigned long long)d) / (unsigned long long)base)
            return STATUS_ERR_OVERFLOW;
        v = v * (unsigned long long)base + (unsigned long long)d;
    }

    *res = neg ? (int)(-(long long)v) : (int)v;
    return STATUS_OK;
}


// %mi %mu %md %mf
void mem_dump(const void* p, size_t size, char* out){
    const unsigned char* bytes = (const unsigned char*)p;
    size_t k = 0;
    for (size_t i = 0; i < size; i++){
        if (i > 0) out[k++] = ' ';
        for (int bit = 7; bit >= 0; bit--)
            out[k++] = ((bytes[i] >> bit) & 1) ? '1' : '0';
    }
    out[k] = '\0';
}


// output string that grows when needed
typedef struct {
    char* data;
    size_t len;
    size_t cap; // always >= len + 1
} strbuf_t;

// add n chars
status_t buf_push(strbuf_t* b, const char* s, size_t n){
    if (b->len + n + 1 > b->cap){
        size_t cap = (b->cap == 0) ? 64 : b->cap;
        while (cap < b->len + n + 1) cap *= 2;

        char* tmp = (char*)realloc(b->data, cap);   // keep the old pointer if it fails
        if (tmp == NULL) return STATUS_ERR_ALLOC;
        b->data = tmp;
        b->cap = cap;
    }
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
    return STATUS_OK;
}


status_t standard_spec(strbuf_t* b, const char* p, va_list* ap, size_t* used){
    char spec[32], tmp[1024];
    int k = 0, i = 0, l = 0;
    spec[k++] = '%';

    while (p[i] != '\0' && strchr("-+ #0123456789.l", p[i]) != NULL){
        if (k >= 28) return STATUS_ERR_FORMAT;
        if (p[i] == 'l') l++;
        else spec[k++] = p[i];
        i++;
    }
    char conv = p[i]; //conversion specifeir
    if (conv == '\0' || l > 2) return STATUS_ERR_FORMAT;
    *used = (size_t)i + 1;

    int is_int = (strchr("diuoxX", conv) != NULL);
    if (is_int){
        spec[k++] = 'l';
        spec[k++] = 'l';
    }
    spec[k++] = conv;
    spec[k] = '\0';

    int n;
    if (conv == 'd' || conv == 'i'){
        long long v = (l == 2) ? va_arg(*ap, long long) : (l == 1) ? va_arg(*ap, long) : va_arg(*ap, int);
        n = snprintf(tmp, sizeof tmp, spec, v);

    } else if (is_int){
        unsigned long long v = (l == 2) ? va_arg(*ap, unsigned long long) :
                               (l == 1) ? va_arg(*ap, unsigned long) : va_arg(*ap, unsigned int);
        n = snprintf(tmp, sizeof tmp, spec, v);

    } else if (conv == 'c'){
        n = snprintf(tmp, sizeof tmp, spec, va_arg(*ap, int));

    } else if (conv == 's'){
        const char* s = va_arg(*ap, const char*);

        if (s == NULL) s = "(null)";
        if (strcmp(spec, "%s") == 0) return buf_push(b, s, strlen(s)); // pushing to dynamic buf
        n = snprintf(tmp, sizeof tmp, spec, s);

    } else if (strchr("fFeEgGaA", conv) != NULL){
        n = snprintf(tmp, sizeof tmp, spec, va_arg(*ap, double));
    } else if (conv == 'p'){
        n = snprintf(tmp, sizeof tmp, spec, va_arg(*ap, void*));
    } else if (conv == 'n'){
        *va_arg(*ap, int*) = (int)b->len;   // number of chars printed so far
        return STATUS_OK;
    } else {
        return STATUS_ERR_FORMAT;
    }

    if (n < 0 || n >= (int)sizeof tmp) return STATUS_ERR_OVERFLOW;
    return buf_push(b, tmp, (size_t)n);
}


status_t custom_spec(strbuf_t* b, const char* p, va_list* ap, int* matched){
    char tmp[80];
    status_t st = STATUS_OK;
    *matched = 1;

    if (p[0] == 'R' && p[1] == 'o'){
        st = roman_str(va_arg(*ap, int), tmp);
    } else if (p[0] == 'Z' && p[1] == 'r'){
        st = zeckendorf_str(va_arg(*ap, unsigned int), tmp);
    } else if (p[0] == 'C' && (p[1] == 'v' || p[1] == 'V')){
        int n = va_arg(*ap, int);
        int base = va_arg(*ap, int);
        st = int_to_base(n, base, p[1] == 'V', tmp);
    } else if ((p[0] == 't' && p[1] == 'o') || (p[0] == 'T' && p[1] == 'O')){
        const char* s = va_arg(*ap, const char*);
        int base = va_arg(*ap, int);
        int v = 0;
        st = base_to_int(s, base, p[0] == 'T', &v);
        if (st == STATUS_OK) st = int_to_base(v, 10, 0, tmp);
    } else if (p[0] == 'm' && p[1] == 'i'){
        int v = va_arg(*ap, int);
        mem_dump(&v, sizeof v, tmp);
    } else if (p[0] == 'm' && p[1] == 'u'){
        unsigned int v = va_arg(*ap, unsigned int);
        mem_dump(&v, sizeof v, tmp);
    } else if (p[0] == 'm' && p[1] == 'd'){
        double v = va_arg(*ap, double);
        mem_dump(&v, sizeof v, tmp);
    } else if (p[0] == 'm' && p[1] == 'f'){
        float v = (float)va_arg(*ap, double);   // float is passed as double
        mem_dump(&v, sizeof v, tmp);
    } else {
        *matched = 0;
        return STATUS_OK;
    }

    if (st != STATUS_OK) return st;
    return buf_push(b, tmp, strlen(tmp));
}


status_t build_output(strbuf_t* b, const char* fmt, va_list* ap){
    status_t st = buf_push(b, "", 0);   // allocates the buffer, so even an empty result has '\0'
    size_t i = 0;

    while (st == STATUS_OK && fmt[i] != '\0'){
        if (fmt[i] != '%'){
            st = buf_push(b, &fmt[i++], 1);
        } else if (fmt[i + 1] == '%'){
            st = buf_push(b, "%", 1);
            i += 2;
        } else {
            int matched = 0;
            size_t used = 0;
            st = custom_spec(b, &fmt[i + 1], ap, &matched);
            if (st == STATUS_OK && matched){
                used = 2;
            } else if (st == STATUS_OK){
                st = standard_spec(b, &fmt[i + 1], ap, &used);
            }
            i += 1 + used;
        }
    }
    return st;
}


int overfprintf(FILE* f, const char* fmt, ...){
    if (f == NULL || fmt == NULL) return -(int)STATUS_ERR_NULL;

    strbuf_t b = {NULL, 0, 0};
    va_list ap;
    va_start(ap, fmt);
    status_t st = build_output(&b, fmt, &ap);
    va_end(ap);

    if (st == STATUS_OK && b.len > INT_MAX) st = STATUS_ERR_OVERFLOW;
    if (st == STATUS_OK && fwrite(b.data, 1, b.len, f) != b.len) st = STATUS_ERR_IO;

    int res = (st == STATUS_OK) ? (int)b.len : -(int)st;
    free(b.data);
    return res;
}

int oversprintf(char* buf, const char* fmt, ...){
    if (buf == NULL || fmt == NULL) return -(int)STATUS_ERR_NULL;

    strbuf_t b = {NULL, 0, 0};
    va_list ap;
    va_start(ap, fmt);
    status_t st = build_output(&b, fmt, &ap);
    va_end(ap);

    if (st == STATUS_OK && b.len > INT_MAX) st = STATUS_ERR_OVERFLOW;
    if (st == STATUS_OK) memcpy(buf, b.data, b.len + 1);

    int res = (st == STATUS_OK) ? (int)b.len : -(int)st;
    free(b.data);
    return res;
}


// output
void print_status(int res){
    switch ((status_t)(res < 0 ? -res : 0)){
        case STATUS_OK:           break;
        case STATUS_ERR_NULL:     printf("Error: NULL pointer\n"); break;
        case STATUS_ERR_FORMAT:   printf("Error: unknown or unsupported specifier\n"); break;
        case STATUS_ERR_RANGE:    printf("Error: value out of range\n"); break;
        case STATUS_ERR_DIGIT:    printf("Error: invalid digit for this base\n"); break;
        case STATUS_ERR_OVERFLOW: printf("Error: number or specifier is too long\n"); break;
        case STATUS_ERR_IO:       printf("Error: failed to write to file\n"); break;
        case STATUS_ERR_ALLOC:    printf("Error: failed to allocate memory\n"); break;
        default:                  printf("Error: unknown error\n"); break;
    }
}

#endif
