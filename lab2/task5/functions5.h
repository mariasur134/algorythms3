#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

// sizes of the small stack buffers (see explanation file)
#define ROMAN_MAX 16    // MMMDCCCLXXXVIII + '\0'
#define ZR_FIB_MAX 48   // fibonacci numbers up to UINT_MAX
#define ZR_MAX 64       // zeckendorf string + '\0'
#define BASE_MAX 40     // sign + 32 binary digits + '\0'
#define DUMP_MAX 80     // 8 bytes * (8 bits + space)
#define SPEC_MAX 48     // one standard spec like "%-+10.3jd"

typedef enum {
    STATUS_OK,            // ok
    STATUS_ERR_NULL,      // NULL pointer
    STATUS_ERR_FORMAT,    // unknown / broken / unsupported specifier
    STATUS_ERR_RANGE,     // value out of range (roman 1..3999, zeckendorf > 0, dump size)
    STATUS_ERR_DIGIT,     // empty string or bad digit for %to / %TO
    STATUS_ERR_OVERFLOW,  // result does not fit in int / size_t
    STATUS_ERR_IO,        // write to file failed
    STATUS_ERR_ALLOC      // malloc / realloc failed
} status_t;


/* ---------- converters: value -> small string ---------- */

// base out of [2..36] -> 10
int norm_base(int base){
    return (base < 2 || base > 36) ? 10 : base;
}

// digit value -> char
char digit_char(int d, int upper){
    if (d < 10) return (char)('0' + d);
    return (char)((upper ? 'A' : 'a') + d - 10);
}

// char -> digit value, letters only in the requested case, -1 if not a digit
int digit_val(char c, int upper){
    if (c >= '0' && c <= '9') return c - '0';
    if (upper && c >= 'A' && c <= 'Z') return c - 'A' + 10;
    if (!upper && c >= 'a' && c <= 'z') return c - 'a' + 10;
    return -1;
}


// %Ro: out must have ROMAN_MAX chars
status_t roman_str(int n, char* out){
    if (out == NULL) return STATUS_ERR_NULL;
    if (n < 1 || n > 3999) return STATUS_ERR_RANGE;

    static const int vals[13] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
    static const char* syms[13] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};

    size_t k = 0;
    for (int i = 0; i < 13; i++){
        while (n >= vals[i]){
            for (int j = 0; syms[i][j] != '\0'; j++) out[k++] = syms[i][j];
            n -= vals[i];
        }
    }
    out[k] = '\0';
    return STATUS_OK;
}


// %Zr: out must have ZR_MAX chars; 0 has no zeckendorf representation
status_t zeckendorf_str(unsigned int n, char* out){
    if (out == NULL) return STATUS_ERR_NULL;
    if (n == 0) return STATUS_ERR_RANGE;

    // 1, 2, 3, 5, 8, ... all numbers <= n
    unsigned long long fib[ZR_FIB_MAX];
    size_t k = 0;
    unsigned long long a = 1, b = 2;
    while (a <= n){
        fib[k++] = a;
        unsigned long long t = a + b;
        a = b;
        b = t;
    }

    // greedy: from the biggest to the smallest, position i <-> fib[i]
    unsigned long long rest = n;
    for (size_t i = k; i > 0; i--){
        if (rest >= fib[i - 1]){
            out[i - 1] = '1';
            rest -= fib[i - 1];
        } else {
            out[i - 1] = '0';
        }
    }
    out[k] = '1';       // end marker
    out[k + 1] = '\0';
    return STATUS_OK;
}


// %Cv / %CV: out must have BASE_MAX chars
status_t int_to_base(int n, int base, int upper, char* out){
    if (out == NULL) return STATUS_ERR_NULL;
    base = norm_base(base);

    long long v = n;   // long long, because -INT_MIN does not fit in int
    int neg = (v < 0);
    if (neg) v = -v;

    char tmp[BASE_MAX];
    size_t k = 0;
    if (v == 0) tmp[k++] = '0';
    while (v > 0){
        tmp[k++] = digit_char((int)(v % base), upper);   // digits come right-to-left
        v /= base;
    }

    size_t p = 0;
    if (neg) out[p++] = '-';
    while (k > 0) out[p++] = tmp[--k];                   // reverse
    out[p] = '\0';
    return STATUS_OK;
}


// %to / %TO: string in base -> int
status_t base_to_int(const char* s, int base, int upper, int* res){
    if (s == NULL || res == NULL) return STATUS_ERR_NULL;
    base = norm_base(base);

    size_t i = 0;
    int neg = 0;
    if (s[0] == '-'){
        neg = 1;
        i = 1;
    }
    if (s[i] == '\0') return STATUS_ERR_DIGIT;

    unsigned long long v = 0;
    for (; s[i] != '\0'; i++){
        int d = digit_val(s[i], upper);
        if (d < 0 || d >= base) return STATUS_ERR_DIGIT;
        v = v * (unsigned long long)base + (unsigned long long)d;
        if (v > (unsigned long long)INT_MAX + 1) return STATUS_ERR_OVERFLOW;
    }

    if (!neg && v > (unsigned long long)INT_MAX) return STATUS_ERR_OVERFLOW;
    *res = neg ? (int)(-(long long)v) : (int)v;
    return STATUS_OK;
}


// %mi %mu %md %mf: bytes in memory order, 8 bits each, separated by one space
status_t mem_dump(const void* p, size_t size, char* out){
    if (p == NULL || out == NULL) return STATUS_ERR_NULL;
    if (size < 1 || size > 8) return STATUS_ERR_RANGE;

    const unsigned char* bytes = (const unsigned char*)p;
    size_t k = 0;
    for (size_t i = 0; i < size; i++){
        if (i > 0) out[k++] = ' ';
        for (int bit = 7; bit >= 0; bit--)
            out[k++] = ((bytes[i] >> bit) & 1) ? '1' : '0';
    }
    out[k] = '\0';
    return STATUS_OK;
}


/* ---------- growing output buffer ---------- */

typedef struct {
    char* data;
    size_t len;   // used chars (without '\0')
    size_t cap;   // allocated bytes
} strbuf_t;

status_t buf_init(strbuf_t* b){
    if (b == NULL) return STATUS_ERR_NULL;
    b->cap = 64;
    b->len = 0;
    b->data = (char*)malloc(b->cap);
    if (b->data == NULL) return STATUS_ERR_ALLOC;
    b->data[0] = '\0';
    return STATUS_OK;
}

void buf_free(strbuf_t* b){
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

// make room for extra more chars + '\0'
status_t buf_reserve(strbuf_t* b, size_t extra){
    if (extra > SIZE_MAX / 2 - b->len - 1) return STATUS_ERR_OVERFLOW;
    size_t need = b->len + extra + 1;
    if (need <= b->cap) return STATUS_OK;

    size_t ncap = b->cap;
    while (ncap < need) ncap *= 2;

    char* tmp = (char*)realloc(b->data, ncap);   // not directly into b->data: keep old pointer on failure
    if (tmp == NULL) return STATUS_ERR_ALLOC;
    b->data = tmp;
    b->cap = ncap;
    return STATUS_OK;
}

status_t buf_push_n(strbuf_t* b, const char* s, size_t n){
    status_t st = buf_reserve(b, n);
    if (st != STATUS_OK) return st;
    for (size_t i = 0; i < n; i++) b->data[b->len + i] = s[i];
    b->len += n;
    b->data[b->len] = '\0';
    return STATUS_OK;
}

status_t buf_push_str(strbuf_t* b, const char* s){
    size_t n = 0;
    while (s[n] != '\0') n++;
    return buf_push_n(b, s, n);
}


/* ---------- standard specifiers: build one spec, call snprintf ---------- */

typedef enum { LEN_NONE, LEN_HH, LEN_H, LEN_L, LEN_LL, LEN_Z, LEN_J, LEN_T, LEN_BIGL } len_t;

// every append_fmt_* measures the result with snprintf(NULL, 0, ...) and prints it for real
status_t append_fmt_imax(strbuf_t* b, const char* spec, intmax_t v){
    int n = snprintf(NULL, 0, spec, v);
    if (n < 0) return STATUS_ERR_FORMAT;
    status_t st = buf_reserve(b, (size_t)n);
    if (st != STATUS_OK) return st;
    snprintf(b->data + b->len, (size_t)n + 1, spec, v);
    b->len += (size_t)n;
    return STATUS_OK;
}

status_t append_fmt_umax(strbuf_t* b, const char* spec, uintmax_t v){
    int n = snprintf(NULL, 0, spec, v);
    if (n < 0) return STATUS_ERR_FORMAT;
    status_t st = buf_reserve(b, (size_t)n);
    if (st != STATUS_OK) return st;
    snprintf(b->data + b->len, (size_t)n + 1, spec, v);
    b->len += (size_t)n;
    return STATUS_OK;
}

status_t append_fmt_int(strbuf_t* b, const char* spec, int v){
    int n = snprintf(NULL, 0, spec, v);
    if (n < 0) return STATUS_ERR_FORMAT;
    status_t st = buf_reserve(b, (size_t)n);
    if (st != STATUS_OK) return st;
    snprintf(b->data + b->len, (size_t)n + 1, spec, v);
    b->len += (size_t)n;
    return STATUS_OK;
}

status_t append_fmt_dbl(strbuf_t* b, const char* spec, double v){
    int n = snprintf(NULL, 0, spec, v);
    if (n < 0) return STATUS_ERR_FORMAT;
    status_t st = buf_reserve(b, (size_t)n);
    if (st != STATUS_OK) return st;
    snprintf(b->data + b->len, (size_t)n + 1, spec, v);
    b->len += (size_t)n;
    return STATUS_OK;
}

status_t append_fmt_ldbl(strbuf_t* b, const char* spec, long double v){
    int n = snprintf(NULL, 0, spec, v);
    if (n < 0) return STATUS_ERR_FORMAT;
    status_t st = buf_reserve(b, (size_t)n);
    if (st != STATUS_OK) return st;
    snprintf(b->data + b->len, (size_t)n + 1, spec, v);
    b->len += (size_t)n;
    return STATUS_OK;
}

status_t append_fmt_ptr(strbuf_t* b, const char* spec, const void* v){
    int n = snprintf(NULL, 0, spec, v);
    if (n < 0) return STATUS_ERR_FORMAT;
    status_t st = buf_reserve(b, (size_t)n);
    if (st != STATUS_OK) return st;
    snprintf(b->data + b->len, (size_t)n + 1, spec, v);
    b->len += (size_t)n;
    return STATUS_OK;
}

// "%s" with NULL is undefined in C, so we print "(null)" like glibc does
status_t append_fmt_str(strbuf_t* b, const char* spec, const char* v){
    if (v == NULL) v = "(null)";
    int n = snprintf(NULL, 0, spec, v);
    if (n < 0) return STATUS_ERR_FORMAT;
    status_t st = buf_reserve(b, (size_t)n);
    if (st != STATUS_OK) return st;
    snprintf(b->data + b->len, (size_t)n + 1, spec, v);
    b->len += (size_t)n;
    return STATUS_OK;
}


// va_arg by length modifier; value is cut to the real type first (like %hhd does)
status_t fetch_signed(va_list* ap, len_t len, intmax_t* v){
    switch (len){
        case LEN_HH:   *v = (signed char)va_arg(*ap, int); break;
        case LEN_H:    *v = (short)va_arg(*ap, int); break;
        case LEN_NONE: *v = va_arg(*ap, int); break;
        case LEN_L:    *v = va_arg(*ap, long); break;
        case LEN_LL:   *v = va_arg(*ap, long long); break;
        case LEN_Z:    *v = (intmax_t)va_arg(*ap, size_t); break;
        case LEN_J:    *v = va_arg(*ap, intmax_t); break;
        case LEN_T:    *v = va_arg(*ap, ptrdiff_t); break;
        default:       return STATUS_ERR_FORMAT;
    }
    return STATUS_OK;
}

status_t fetch_unsigned(va_list* ap, len_t len, uintmax_t* v){
    switch (len){
        case LEN_HH:   *v = (unsigned char)va_arg(*ap, unsigned int); break;
        case LEN_H:    *v = (unsigned short)va_arg(*ap, unsigned int); break;
        case LEN_NONE: *v = va_arg(*ap, unsigned int); break;
        case LEN_L:    *v = va_arg(*ap, unsigned long); break;
        case LEN_LL:   *v = va_arg(*ap, unsigned long long); break;
        case LEN_Z:    *v = va_arg(*ap, size_t); break;
        case LEN_J:    *v = va_arg(*ap, uintmax_t); break;
        case LEN_T:    *v = (uintmax_t)va_arg(*ap, ptrdiff_t); break;
        default:       return STATUS_ERR_FORMAT;
    }
    return STATUS_OK;
}

// helpers for building the spec string
status_t spec_put(char* spec, size_t* k, char c){
    if (*k + 1 >= SPEC_MAX) return STATUS_ERR_FORMAT;
    spec[(*k)++] = c;
    return STATUS_OK;
}

status_t spec_put_num(char* spec, size_t* k, long long num){
    char tmp[24];
    size_t t = 0;
    if (num == 0) tmp[t++] = '0';
    while (num > 0){
        tmp[t++] = (char)('0' + num % 10);
        num /= 10;
    }
    while (t > 0){
        status_t st = spec_put(spec, k, tmp[--t]);
        if (st != STATUS_OK) return st;
    }
    return STATUS_OK;
}

int is_digit_c(char c){
    return c >= '0' && c <= '9';
}

// p points to the char after '%'; parses the whole standard spec and prints one value.
// *used = how many chars of the format were consumed (b->len is what %n stores)
status_t standard_spec(strbuf_t* b, const char* p, va_list* ap, size_t* used){
    char spec[SPEC_MAX];
    size_t k = 0, i = 0;
    status_t st = spec_put(spec, &k, '%');
    if (st != STATUS_OK) return st;

    // flags
    while (p[i] == '-' || p[i] == '+' || p[i] == ' ' || p[i] == '#' || p[i] == '0'){
        st = spec_put(spec, &k, p[i++]);
        if (st != STATUS_OK) return st;
    }

    // width: number or * (value is taken from the stack right now)
    if (p[i] == '*'){
        long long w = va_arg(*ap, int);
        if (w < 0){
            st = spec_put(spec, &k, '-');
            if (st != STATUS_OK) return st;
            w = -w;
        }
        st = spec_put_num(spec, &k, w);
        if (st != STATUS_OK) return st;
        i++;
    } else {
        while (is_digit_c(p[i])){
            st = spec_put(spec, &k, p[i++]);
            if (st != STATUS_OK) return st;
        }
    }

    // precision: .number or .* (negative * means "no precision")
    if (p[i] == '.'){
        i++;
        if (p[i] == '*'){
            long long pr = va_arg(*ap, int);
            if (pr >= 0){
                st = spec_put(spec, &k, '.');
                if (st != STATUS_OK) return st;
                st = spec_put_num(spec, &k, pr);
                if (st != STATUS_OK) return st;
            }
            i++;
        } else {
            st = spec_put(spec, &k, '.');
            if (st != STATUS_OK) return st;
            while (is_digit_c(p[i])){
                st = spec_put(spec, &k, p[i++]);
                if (st != STATUS_OK) return st;
            }
        }
    }

    // length modifier
    len_t len = LEN_NONE;
    if (p[i] == 'h' && p[i + 1] == 'h'){ len = LEN_HH; i += 2; }
    else if (p[i] == 'h'){ len = LEN_H; i++; }
    else if (p[i] == 'l' && p[i + 1] == 'l'){ len = LEN_LL; i += 2; }
    else if (p[i] == 'l'){ len = LEN_L; i++; }
    else if (p[i] == 'z'){ len = LEN_Z; i++; }
    else if (p[i] == 'j'){ len = LEN_J; i++; }
    else if (p[i] == 't'){ len = LEN_T; i++; }
    else if (p[i] == 'L'){ len = LEN_BIGL; i++; }

    char conv = p[i];
    if (conv == '\0') return STATUS_ERR_FORMAT;
    i++;
    *used = i;

    switch (conv){
        case 'd':
        case 'i': {
            intmax_t v = 0;
            st = fetch_signed(ap, len, &v);
            if (st != STATUS_OK) return st;
            // all integer lengths are replaced by 'j': the value is already widened to intmax_t
            if ((st = spec_put(spec, &k, 'j')) != STATUS_OK) return st;
            if ((st = spec_put(spec, &k, conv)) != STATUS_OK) return st;
            spec[k] = '\0';
            return append_fmt_imax(b, spec, v);
        }
        case 'u':
        case 'o':
        case 'x':
        case 'X': {
            uintmax_t v = 0;
            st = fetch_unsigned(ap, len, &v);
            if (st != STATUS_OK) return st;
            if ((st = spec_put(spec, &k, 'j')) != STATUS_OK) return st;
            if ((st = spec_put(spec, &k, conv)) != STATUS_OK) return st;
            spec[k] = '\0';
            return append_fmt_umax(b, spec, v);
        }
        case 'c': {
            if (len != LEN_NONE) return STATUS_ERR_FORMAT;
            int v = va_arg(*ap, int);
            if ((st = spec_put(spec, &k, 'c')) != STATUS_OK) return st;
            spec[k] = '\0';
            return append_fmt_int(b, spec, v);
        }
        case 's': {
            if (len != LEN_NONE) return STATUS_ERR_FORMAT;
            const char* v = va_arg(*ap, const char*);
            if ((st = spec_put(spec, &k, 's')) != STATUS_OK) return st;
            spec[k] = '\0';
            return append_fmt_str(b, spec, v);
        }
        case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': case 'a': case 'A': {
            if (len == LEN_BIGL){
                long double v = va_arg(*ap, long double);
                if ((st = spec_put(spec, &k, 'L')) != STATUS_OK) return st;
                if ((st = spec_put(spec, &k, conv)) != STATUS_OK) return st;
                spec[k] = '\0';
                return append_fmt_ldbl(b, spec, v);
            }
            if (len != LEN_NONE && len != LEN_L) return STATUS_ERR_FORMAT;
            double v = va_arg(*ap, double);
            if ((st = spec_put(spec, &k, conv)) != STATUS_OK) return st;
            spec[k] = '\0';
            return append_fmt_dbl(b, spec, v);
        }
        case 'p': {
            if (len != LEN_NONE) return STATUS_ERR_FORMAT;
            const void* v = va_arg(*ap, const void*);
            if ((st = spec_put(spec, &k, 'p')) != STATUS_OK) return st;
            spec[k] = '\0';
            return append_fmt_ptr(b, spec, v);
        }
        case 'n': {
            // writes the number of chars printed so far
            switch (len){
                case LEN_NONE: *va_arg(*ap, int*) = (int)b->len; break;
                case LEN_HH:   *va_arg(*ap, signed char*) = (signed char)b->len; break;
                case LEN_H:    *va_arg(*ap, short*) = (short)b->len; break;
                case LEN_L:    *va_arg(*ap, long*) = (long)b->len; break;
                case LEN_LL:   *va_arg(*ap, long long*) = (long long)b->len; break;
                default:       return STATUS_ERR_FORMAT;
            }
            return STATUS_OK;
        }
        default:
            return STATUS_ERR_FORMAT;
    }
}


/* ---------- our specifiers ---------- */

// p points to the char after '%'. *matched = 1 if p starts with one of our specifiers (2 chars)
status_t custom_spec(strbuf_t* b, const char* p, va_list* ap, int* matched){
    char tmp[BASE_MAX > DUMP_MAX ? BASE_MAX : DUMP_MAX];
    status_t st = STATUS_OK;
    *matched = 1;

    if (p[0] == 'R' && p[1] == 'o'){
        int n = va_arg(*ap, int);
        st = roman_str(n, tmp);
    } else if (p[0] == 'Z' && p[1] == 'r'){
        unsigned int n = va_arg(*ap, unsigned int);
        st = zeckendorf_str(n, tmp);
    } else if (p[0] == 'C' && (p[1] == 'v' || p[1] == 'V')){
        // two separate statements: the order of evaluation of function arguments is not defined
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
        st = mem_dump(&v, sizeof v, tmp);
    } else if (p[0] == 'm' && p[1] == 'u'){
        unsigned int v = va_arg(*ap, unsigned int);
        st = mem_dump(&v, sizeof v, tmp);
    } else if (p[0] == 'm' && p[1] == 'd'){
        double v = va_arg(*ap, double);
        st = mem_dump(&v, sizeof v, tmp);
    } else if (p[0] == 'm' && p[1] == 'f'){
        float v = (float)va_arg(*ap, double);   // float is passed as double
        st = mem_dump(&v, sizeof v, tmp);
    } else {
        *matched = 0;
        return STATUS_OK;
    }

    if (st != STATUS_OK) return st;
    return buf_push_str(b, tmp);
}


// walks through the format; fills b with the result
status_t build_output(strbuf_t* b, const char* fmt, va_list* ap){
    if (b == NULL || fmt == NULL || ap == NULL) return STATUS_ERR_NULL;

    size_t i = 0;
    while (fmt[i] != '\0'){
        status_t st;

        if (fmt[i] != '%'){
            st = buf_push_n(b, &fmt[i], 1);
            if (st != STATUS_OK) return st;
            i++;
            continue;
        }

        i++;   // skip '%'
        if (fmt[i] == '%'){
            st = buf_push_n(b, "%", 1);
            if (st != STATUS_OK) return st;
            i++;
            continue;
        }

        int matched = 0;
        st = custom_spec(b, &fmt[i], ap, &matched);
        if (st != STATUS_OK) return st;
        if (matched){
            i += 2;
            continue;
        }

        size_t used = 0;
        st = standard_spec(b, &fmt[i], ap, &used);
        if (st != STATUS_OK) return st;
        i += used;
    }
    return STATUS_OK;
}


/* ---------- public functions ---------- */

// returns number of chars written, or -status on error (like fprintf returns < 0)
int overfprintf(FILE* f, const char* fmt, ...){
    if (f == NULL || fmt == NULL) return -(int)STATUS_ERR_NULL;

    strbuf_t b;
    status_t st = buf_init(&b);
    if (st != STATUS_OK) return -(int)st;

    va_list ap;
    va_start(ap, fmt);
    st = build_output(&b, fmt, &ap);
    va_end(ap);

    if (st == STATUS_OK && fwrite(b.data, 1, b.len, f) != b.len) st = STATUS_ERR_IO;
    if (st == STATUS_OK && b.len > (size_t)INT_MAX) st = STATUS_ERR_OVERFLOW;

    int res = (st == STATUS_OK) ? (int)b.len : -(int)st;
    buf_free(&b);
    return res;
}

// like sprintf: buf must be big enough; returns number of chars (without '\0') or -status
int oversprintf(char* buf, const char* fmt, ...){
    if (buf == NULL || fmt == NULL) return -(int)STATUS_ERR_NULL;

    strbuf_t b;
    status_t st = buf_init(&b);
    if (st != STATUS_OK) return -(int)st;

    va_list ap;
    va_start(ap, fmt);
    st = build_output(&b, fmt, &ap);
    va_end(ap);

    if (st == STATUS_OK && b.len > (size_t)INT_MAX) st = STATUS_ERR_OVERFLOW;
    if (st == STATUS_OK){
        for (size_t i = 0; i <= b.len; i++) buf[i] = b.data[i];   // with '\0'
    }

    int res = (st == STATUS_OK) ? (int)b.len : -(int)st;
    buf_free(&b);
    return res;
}


// output
void print_status(int res){
    if (res >= 0) return;

    switch ((status_t)(-res)){
        case STATUS_ERR_NULL:     printf("Error: NULL pointer\n"); break;
        case STATUS_ERR_FORMAT:   printf("Error: unknown or unsupported specifier\n"); break;
        case STATUS_ERR_RANGE:    printf("Error: value out of range\n"); break;
        case STATUS_ERR_DIGIT:    printf("Error: invalid digit for this base\n"); break;
        case STATUS_ERR_OVERFLOW: printf("Error: number does not fit in int\n"); break;
        case STATUS_ERR_IO:       printf("Error: failed to write to file\n"); break;
        case STATUS_ERR_ALLOC:    printf("Error: failed to allocate memory\n"); break;
        default:                  printf("Error: unknown error\n"); break;
    }
}

#endif
