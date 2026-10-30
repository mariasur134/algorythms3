#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#define ZECK_MAX_DIGITS 47 // 46 f nums can fit + 1

typedef enum {
    STATUS_OK,           // ok
    STATUS_EOF,
    STATUS_ERR_NULL,
    STATUS_ERR_FORMAT,   // unknown specifier
    STATUS_ERR_MATCH     // input does not match
} status_t;


typedef struct {
    FILE* f;          // if not NULL -> read from file
    const char* s;    // read from string
} reader_t;

int peek(const reader_t* r){
    if (r->f != NULL){
        int c = fgetc(r->f);
        if (c != EOF) ungetc(c, r->f);
        return c;
    }
    return (*r->s == '\0') ? EOF : (unsigned char)*r->s;
}

void take(reader_t* r){
    if (r->f != NULL) fgetc(r->f);
    else r->s++;
}

int is_space(int c){
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

void skip_spaces(reader_t* r){
    while (is_space(peek(r))) take(r);
}

// char -> digit val
int digit_val(int c, int upper){
    if (c >= '0' && c <= '9') return c - '0';
    if (upper && c >= 'A' && c <= 'Z') return c - 'A' + 10;
    if (!upper && c >= 'a' && c <= 'z') return c - 'a' + 10;
    return -1;
}

status_t nothing_read(const reader_t* r){
    return (peek(r) == EOF) ? STATUS_EOF : STATUS_ERR_MATCH;
}


// roman digit -> value, 0 if some bs
int roman_val(int c){
    static const int vals[7] = {1, 5, 10, 50, 100, 500, 1000};
    static const char digits[] = "IVXLCDM";
    const char* q = (c > 0) ? strchr(digits, c) : NULL;
    return (q == NULL) ? 0 : vals[q - digits];
}


// %Ro - reading
status_t scan_roman(reader_t* r, int* out){
    if (out == NULL) return STATUS_ERR_NULL;
    skip_spaces(r);

    int total = 0, prev = 0, cur, digits = 0;
    while ((cur = roman_val(peek(r))) != 0){
        total += (prev < cur) ? cur - 2 * prev : cur; //IV = 1 + 5 - 1 - 1 = 4
        prev = cur;
        digits++;
        take(r);
    }
    if (digits == 0) return nothing_read(r);

    *out = total;
    return STATUS_OK;
}


// %Zr
status_t scan_zeckendorf(reader_t* r, unsigned int* out){
    if (out == NULL) return STATUS_ERR_NULL;
    skip_spaces(r);

    unsigned long long a = 1, b = 2, sum = 0;
    int c, prev = 0, digits = 0;
    while ((c = peek(r)) == '0' || c == '1'){
        take(r);
        digits++;
        if (prev == '1' && c == '1'){          // marker: sum is ready
            *out = (unsigned int)sum;
            return STATUS_OK;
        }
        if (c == '1') sum += a;
        if (sum > UINT_MAX || digits > ZECK_MAX_DIGITS) return STATUS_ERR_MATCH;

        unsigned long long t = a + b;
        a = b;
        b = t;
        prev = c;
    }
    return (digits == 0) ? nothing_read(r) : STATUS_ERR_MATCH;   // no end marker
}


// %Cv %CV
status_t scan_base(reader_t* r, int* out, int base, int upper){
    if (out == NULL) return STATUS_ERR_NULL;
    if (base < 2 || base > 36) base = 10;
    skip_spaces(r);

    int neg = (peek(r) == '-');
    if (neg) take(r);


    unsigned long long limit = neg ? (unsigned long long)INT_MAX + 1 : (unsigned long long)INT_MAX;
    unsigned long long v = 0;
    int c, d, digits = 0;

    while ((c = peek(r)) != EOF && (d = digit_val(c, upper)) >= 0 && d < base){
        // v <= (limit - d) / base
        if (v > (limit - (unsigned long long)d) / (unsigned long long)base)
            return STATUS_ERR_MATCH;
        v = v * (unsigned long long)base + (unsigned long long)d;
        digits++;
        take(r);
    }
    if (digits == 0) return (neg) ? STATUS_ERR_MATCH : nothing_read(r);

    *out = neg ? (int)(-(long long)v) : (int)v;
    return STATUS_OK;
}

// our specifiers; p = char after '%'; *matched = 0 if p is not one of them
status_t custom_scan(reader_t* r, const char* p, va_list* ap, int* matched, int* count){
    status_t st;
    *matched = 1;
    if (p[0] == 'R' && p[1] == 'o'){
        st = scan_roman(r, va_arg(*ap, int*));
    } else if (p[0] == 'Z' && p[1] == 'r'){
        st = scan_zeckendorf(r, va_arg(*ap, unsigned int*));
    } else if (p[0] == 'C' && (p[1] == 'v' || p[1] == 'V')){
        // address first, base second; two statements, because the order of evaluating arguments is not defined
        int* out = va_arg(*ap, int*);
        int base = va_arg(*ap, int);
        st = scan_base(r, out, base, p[1] == 'V');
    } else {
        *matched = 0;
        return STATUS_OK;
    }

    if (st == STATUS_OK) (*count)++;
    return st;
}


status_t standard_scan(reader_t* r, const char* p, va_list* ap, size_t* used, int* count){
    char spec[32];
    size_t i = 0;
    spec[0] = '%';

    while (p[i] != '\0' && strchr("*0123456789hlL", p[i]) != NULL){
        if (i >= 26) return STATUS_ERR_FORMAT;
        spec[i + 1] = p[i];
        i++;
    }
    if (p[i] == '\0' || strchr("diouxXeEfFgGcsp", p[i]) == NULL) return STATUS_ERR_FORMAT;
    spec[i + 1] = p[i];
    strcpy(spec + i + 2, "%n");     // %n - how many read
    *used = i + 1;

    int suppress = (strchr(spec, '*') != NULL);   // '*': read, but do not store
    void* target = suppress ? NULL : va_arg(*ap, void*);
    if (!suppress && target == NULL) return STATUS_ERR_NULL;

    int n = -1, res;                               // n stays -1 -> failed
    if (r->f != NULL) res = suppress ? fscanf(r->f, spec, &n) : fscanf(r->f, spec, target, &n);
    else res = suppress ? sscanf(r->s, spec, &n) : sscanf(r->s, spec, target, &n);

    if (n < 0) return (res == EOF) ? STATUS_EOF : STATUS_ERR_MATCH;
    if (r->f == NULL) r->s += n;
    if (!suppress) (*count)++;
    return STATUS_OK;
}


status_t scan_input(reader_t* r, const char* fmt, va_list* ap, int* count){
    *count = 0;
    size_t i = 0;

    while (fmt[i] != '\0'){
        if (is_space(fmt[i])){
            skip_spaces(r);
            i++;
        } else if (fmt[i] != '%' || fmt[i + 1] == '%'){
            if (fmt[i] == '%') skip_spaces(r);
            int c = peek(r);
            if (c == EOF) return STATUS_EOF;
            if (c != (unsigned char)fmt[i]) return STATUS_ERR_MATCH;
            take(r);
            i += (fmt[i] == '%') ? 2 : 1;
        } else {
            int matched = 0;
            size_t used = 0;
            status_t st = custom_scan(r, &fmt[i + 1], ap, &matched, count);
            if (st == STATUS_OK && matched) used = 2;
            else if (st == STATUS_OK) st = standard_scan(r, &fmt[i + 1], ap, &used, count);
            if (st != STATUS_OK) return st;

            i += 1 + used;
        }
    }
    return STATUS_OK;
}

int finish_scan(status_t st, int count){
    if (st == STATUS_EOF && count == 0) return EOF;
    if (st == STATUS_ERR_NULL || st == STATUS_ERR_FORMAT) return -(int)st;
    return count;
}

int overfscanf(FILE* f, const char* fmt, ...){
    if (f == NULL || fmt == NULL) return -(int)STATUS_ERR_NULL;

    reader_t r = {f, NULL};
    int count;
    va_list ap;
    va_start(ap, fmt);
    status_t st = scan_input(&r, fmt, &ap, &count);
    va_end(ap);
    return finish_scan(st, count);
}

int oversscanf(const char* str, const char* fmt, ...){
    if (str == NULL || fmt == NULL) return -(int)STATUS_ERR_NULL;

    reader_t r = {NULL, str};
    int count;
    va_list ap;
    va_start(ap, fmt);
    status_t st = scan_input(&r, fmt, &ap, &count);
    va_end(ap);
    return finish_scan(st, count);
}


void print_scan_result(int res){
    if (res >= 0) printf("Stored values: %d\n", res);
    else if (res == EOF) printf("Error: input ended before the first value\n");
    else if (-res == STATUS_ERR_NULL) printf("Error: NULL pointer\n");
    else printf("Error: unknown or unsupported specifier\n");
}

#endif
