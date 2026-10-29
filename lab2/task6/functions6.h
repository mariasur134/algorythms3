#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <limits.h>

// sizes of the token buffers (see explanation file)
#define ROMAN_TOK_MAX 32   // roman digits of one number
#define ZR_TOK_MAX 64      // zeckendorf digits + end marker
#define BASE_TOK_MAX 40    // sign + 32 digits + '\0'
#define SPEC_MAX 48        // one standard spec like "%*5ld%n"

// STATUS_EOF is 1 on purpose: overfscanf returns -STATUS_EOF == EOF, like scanf does
typedef enum {
    STATUS_OK,            // ok
    STATUS_EOF,           // input ended before the conversion
    STATUS_ERR_NULL,      // NULL pointer
    STATUS_ERR_FORMAT,    // unknown / broken / unsupported specifier
    STATUS_ERR_MATCH,     // input does not match the format
    STATUS_ERR_OVERFLOW   // token too long or value does not fit
} status_t;


/* ---------- helpers ---------- */

int is_space_c(int c){
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int is_digit_c(int c){
    return c >= '0' && c <= '9';
}

// base out of [2..36] -> 10
int norm_base(int base){
    return (base < 2 || base > 36) ? 10 : base;
}

// char -> digit value, letters only in the requested case, -1 if not a digit
int digit_val(int c, int upper){
    if (c >= '0' && c <= '9') return c - '0';
    if (upper && c >= 'A' && c <= 'Z') return c - 'A' + 10;
    if (!upper && c >= 'a' && c <= 'z') return c - 'a' + 10;
    return -1;
}

// roman digit -> value, 0 if it is not a roman digit
int roman_val(int c){
    switch (c){
        case 'I': return 1;
        case 'V': return 5;
        case 'X': return 10;
        case 'L': return 50;
        case 'C': return 100;
        case 'D': return 500;
        case 'M': return 1000;
        default:  return 0;
    }
}


/* ---------- converters: small string -> value ---------- */

// %Ro: s contains only roman digits
status_t roman_to_int(const char* s, int* res){
    if (s == NULL || res == NULL) return STATUS_ERR_NULL;
    if (s[0] == '\0') return STATUS_ERR_MATCH;

    size_t n = 0;
    while (s[n] != '\0') n++;

    // from right to left: a digit smaller than the previous one is subtracted
    long long v = 0;
    int prev = 0;
    for (size_t i = n; i > 0; i--){
        int cur = roman_val(s[i - 1]);
        if (cur == 0) return STATUS_ERR_MATCH;
        if (cur < prev) v -= cur;
        else v += cur;
        prev = cur;
    }
    if (v > INT_MAX || v < INT_MIN) return STATUS_ERR_OVERFLOW;

    *res = (int)v;
    return STATUS_OK;
}


// %Zr: s = coefficients (0/1) from the smallest fibonacci number, the last char is the end marker
status_t zeckendorf_to_uint(const char* s, unsigned int* res){
    if (s == NULL || res == NULL) return STATUS_ERR_NULL;

    size_t n = 0;
    while (s[n] != '\0') n++;
    if (n < 2 || s[n - 1] != '1') return STATUS_ERR_MATCH;

    unsigned long long a = 1, b = 2, v = 0;   // a = current fibonacci number: 1, 2, 3, 5, ...
    for (size_t i = 0; i + 1 < n; i++){
        if (s[i] == '1') v += a;
        else if (s[i] != '0') return STATUS_ERR_MATCH;
        if (v > UINT_MAX) return STATUS_ERR_OVERFLOW;

        unsigned long long t = a + b;
        a = b;
        b = t;
    }

    *res = (unsigned int)v;
    return STATUS_OK;
}


// %Cv / %CV: s = [-]digits in base
status_t base_to_int(const char* s, int base, int upper, int* res){
    if (s == NULL || res == NULL) return STATUS_ERR_NULL;
    base = norm_base(base);

    size_t i = 0;
    int neg = 0;
    if (s[0] == '-'){
        neg = 1;
        i = 1;
    }
    if (s[i] == '\0') return STATUS_ERR_MATCH;

    unsigned long long v = 0;
    for (; s[i] != '\0'; i++){
        int d = digit_val(s[i], upper);
        if (d < 0 || d >= base) return STATUS_ERR_MATCH;
        v = v * (unsigned long long)base + (unsigned long long)d;
        if (v > (unsigned long long)INT_MAX + 1) return STATUS_ERR_OVERFLOW;
    }

    if (!neg && v > (unsigned long long)INT_MAX) return STATUS_ERR_OVERFLOW;
    *res = neg ? (int)(-(long long)v) : (int)v;
    return STATUS_OK;
}


/* ---------- reader: one interface for a file and for a string ---------- */

typedef struct {
    FILE* f;           // != NULL: read from the file
    const char* s;     // else read from the string
    size_t pos;        // position in the string
    size_t consumed;   // chars taken from the input so far (for %n)
} reader_t;

// next char without taking it, EOF if the input ended
int reader_peek(const reader_t* r){
    if (r->f != NULL){
        int c = fgetc(r->f);
        if (c != EOF) ungetc(c, r->f);
        return c;
    }
    return (r->s[r->pos] == '\0') ? EOF : (unsigned char)r->s[r->pos];
}

// take the next char (call only after reader_peek returned not EOF)
void reader_next(reader_t* r){
    if (r->f != NULL) fgetc(r->f);
    else r->pos++;
    r->consumed++;
}

void reader_skip_ws(reader_t* r){
    while (is_space_c(reader_peek(r))) reader_next(r);
}

// what to report if a token turned out to be empty
status_t empty_token_status(const reader_t* r){
    return (reader_peek(r) == EOF) ? STATUS_EOF : STATUS_ERR_MATCH;
}


/* ---------- our specifiers ---------- */

status_t scan_roman(reader_t* r, int* out){
    if (out == NULL) return STATUS_ERR_NULL;

    reader_skip_ws(r);
    char tok[ROMAN_TOK_MAX];
    size_t n = 0;
    while (roman_val(reader_peek(r)) != 0){
        if (n + 1 >= ROMAN_TOK_MAX) return STATUS_ERR_OVERFLOW;
        tok[n++] = (char)reader_peek(r);
        reader_next(r);
    }
    if (n == 0) return empty_token_status(r);
    tok[n] = '\0';

    return roman_to_int(tok, out);
}

status_t scan_zeckendorf(reader_t* r, unsigned int* out){
    if (out == NULL) return STATUS_ERR_NULL;

    reader_skip_ws(r);
    char tok[ZR_TOK_MAX];
    size_t n = 0;
    int done = 0;
    int c;
    while (!done && ((c = reader_peek(r)) == '0' || c == '1')){
        if (n + 1 >= ZR_TOK_MAX) return STATUS_ERR_OVERFLOW;
        tok[n++] = (char)c;
        reader_next(r);
        // two '1' in a row: the second one is the end marker
        if (n >= 2 && tok[n - 1] == '1' && tok[n - 2] == '1') done = 1;
    }
    if (n == 0) return empty_token_status(r);
    if (!done) return STATUS_ERR_MATCH;   // no end marker
    tok[n] = '\0';

    return zeckendorf_to_uint(tok, out);
}

status_t scan_base(reader_t* r, int* out, int base, int upper){
    if (out == NULL) return STATUS_ERR_NULL;
    base = norm_base(base);

    reader_skip_ws(r);
    char tok[BASE_TOK_MAX];
    size_t n = 0;
    if (reader_peek(r) == '-'){
        tok[n++] = '-';
        reader_next(r);
    }

    size_t digits = 0;
    int c;
    while ((c = reader_peek(r)) != EOF && digit_val(c, upper) >= 0 && digit_val(c, upper) < base){
        if (n + 1 >= BASE_TOK_MAX) return STATUS_ERR_OVERFLOW;
        tok[n++] = (char)c;
        digits++;
        reader_next(r);
    }
    if (digits == 0) return (n > 0) ? STATUS_ERR_MATCH : empty_token_status(r);   // a lone '-' is a bad number, not the end of input
    tok[n] = '\0';

    return base_to_int(tok, base, upper, out);
}

// p points to the char after '%'. *matched = 1 if p starts with one of our specifiers (2 chars)
// *count = number of values stored so far (increased if this specifier stored one)
status_t custom_scan(reader_t* r, const char* p, va_list* ap, int* matched, int* count){
    status_t st;
    *matched = 1;

    if (p[0] == 'R' && p[1] == 'o'){
        int* out = va_arg(*ap, int*);
        st = scan_roman(r, out);
    } else if (p[0] == 'Z' && p[1] == 'r'){
        unsigned int* out = va_arg(*ap, unsigned int*);
        st = scan_zeckendorf(r, out);
    } else if (p[0] == 'C' && (p[1] == 'v' || p[1] == 'V')){
        // two separate statements: the order of evaluation of function arguments is not defined
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


/* ---------- standard specifiers: build one spec, call fscanf / sscanf ---------- */

status_t spec_put(char* spec, size_t* k, char c){
    if (*k + 1 >= SPEC_MAX) return STATUS_ERR_FORMAT;
    spec[(*k)++] = c;
    return STATUS_OK;
}

// p points to the char after '%'; *used = how many chars of the format were consumed
status_t standard_scan(reader_t* r, const char* p, va_list* ap, size_t* used, int* count){
    char spec[SPEC_MAX];
    size_t k = 0, i = 0;
    status_t st = spec_put(spec, &k, '%');
    if (st != STATUS_OK) return st;

    // '*' = read but do not store
    int suppress = 0;
    if (p[i] == '*'){
        suppress = 1;
        if ((st = spec_put(spec, &k, p[i++])) != STATUS_OK) return st;
    }

    // width
    while (is_digit_c(p[i])){
        if ((st = spec_put(spec, &k, p[i++])) != STATUS_OK) return st;
    }

    // length modifier
    size_t len_chars = 0;
    while (p[i] == 'h' || p[i] == 'l' || p[i] == 'L' || p[i] == 'z' || p[i] == 'j' || p[i] == 't'){
        if ((st = spec_put(spec, &k, p[i++])) != STATUS_OK) return st;
        len_chars++;
    }

    char conv = p[i];
    switch (conv){
        case 'd': case 'i': case 'u': case 'o': case 'x': case 'X':
        case 'e': case 'E': case 'f': case 'F': case 'g': case 'G': case 'a': case 'A':
        case 'c': case 's': case 'p': case 'n':
            if ((st = spec_put(spec, &k, conv)) != STATUS_OK) return st;
            i++;
            break;
        case '[':
            // %[set]: the set ends at the first ']' (a ']' right after '[' or '[^' belongs to the set)
            if ((st = spec_put(spec, &k, p[i++])) != STATUS_OK) return st;
            if (p[i] == '^' && (st = spec_put(spec, &k, p[i++])) != STATUS_OK) return st;
            if (p[i] == ']' && (st = spec_put(spec, &k, p[i++])) != STATUS_OK) return st;
            while (p[i] != '\0' && p[i] != ']'){
                if ((st = spec_put(spec, &k, p[i++])) != STATUS_OK) return st;
            }
            if (p[i] != ']') return STATUS_ERR_FORMAT;
            if ((st = spec_put(spec, &k, p[i++])) != STATUS_OK) return st;
            break;
        default:
            return STATUS_ERR_FORMAT;
    }
    *used = i;

    // %n: how many chars were read so far (we know it, the real scanf does not)
    if (conv == 'n'){
        if (len_chars != 0) return STATUS_ERR_FORMAT;
        if (!suppress){
            int* out = va_arg(*ap, int*);
            if (out == NULL) return STATUS_ERR_NULL;
            *out = (int)r->consumed;
        }
        return STATUS_OK;
    }

    // add %n at the end: it tells how many chars this one conversion has read
    if ((st = spec_put(spec, &k, '%')) != STATUS_OK) return st;
    if ((st = spec_put(spec, &k, 'n')) != STATUS_OK) return st;
    spec[k] = '\0';

    void* target = NULL;
    if (!suppress){
        target = va_arg(*ap, void*);
        if (target == NULL) return STATUS_ERR_NULL;
    }

    int n = -1;   // stays -1 if the conversion failed
    int res;
    if (r->f != NULL){
        res = suppress ? fscanf(r->f, spec, &n) : fscanf(r->f, spec, target, &n);
    } else {
        res = suppress ? sscanf(r->s + r->pos, spec, &n) : sscanf(r->s + r->pos, spec, target, &n);
    }

    if (n < 0) return (res == EOF) ? STATUS_EOF : STATUS_ERR_MATCH;

    if (r->f == NULL) r->pos += (size_t)n;
    r->consumed += (size_t)n;
    if (!suppress) (*count)++;
    return STATUS_OK;
}


/* ---------- main loop ---------- */

// walks through the format and reads the input; *count = number of stored values
status_t scan_input(reader_t* r, const char* fmt, va_list* ap, int* count){
    if (r == NULL || fmt == NULL || ap == NULL || count == NULL) return STATUS_ERR_NULL;
    *count = 0;

    size_t i = 0;
    while (fmt[i] != '\0'){
        status_t st;

        // a space in the format = any number of spaces in the input
        if (is_space_c((unsigned char)fmt[i])){
            reader_skip_ws(r);
            i++;
            continue;
        }

        // any other char must be in the input as it is
        if (fmt[i] != '%'){
            int c = reader_peek(r);
            if (c == EOF) return STATUS_EOF;
            if (c != (unsigned char)fmt[i]) return STATUS_ERR_MATCH;
            reader_next(r);
            i++;
            continue;
        }

        i++;   // skip '%'
        if (fmt[i] == '%'){
            reader_skip_ws(r);
            int c = reader_peek(r);
            if (c == EOF) return STATUS_EOF;
            if (c != '%') return STATUS_ERR_MATCH;
            reader_next(r);
            i++;
            continue;
        }

        int matched = 0;
        st = custom_scan(r, &fmt[i], ap, &matched, count);
        if (st != STATUS_OK) return st;
        if (matched){
            i += 2;
            continue;
        }

        size_t used = 0;
        st = standard_scan(r, &fmt[i], ap, &used, count);
        if (st != STATUS_OK) return st;
        i += used;
    }
    return STATUS_OK;
}


/* ---------- public functions ---------- */

// result of scan_input -> return value of overfscanf / oversscanf:
// count of stored values; EOF if the input ended before the first one; -status if the call itself is wrong
int finish_scan(status_t st, int count){
    switch (st){
        case STATUS_OK:
        case STATUS_ERR_MATCH:
        case STATUS_ERR_OVERFLOW:
            return count;               // stopped on the first bad token, like scanf
        case STATUS_EOF:
            return (count == 0) ? EOF : count;
        default:
            return -(int)st;            // NULL or broken format
    }
}

int overfscanf(FILE* f, const char* fmt, ...){
    if (f == NULL || fmt == NULL) return -(int)STATUS_ERR_NULL;

    reader_t r = {f, NULL, 0, 0};
    int count = 0;

    va_list ap;
    va_start(ap, fmt);
    status_t st = scan_input(&r, fmt, &ap, &count);
    va_end(ap);

    return finish_scan(st, count);
}

int oversscanf(const char* str, const char* fmt, ...){
    if (str == NULL || fmt == NULL) return -(int)STATUS_ERR_NULL;

    reader_t r = {NULL, str, 0, 0};
    int count = 0;

    va_list ap;
    va_start(ap, fmt);
    status_t st = scan_input(&r, fmt, &ap, &count);
    va_end(ap);

    return finish_scan(st, count);
}


// output
void print_scan_result(int res){
    if (res >= 0){
        printf("Stored values: %d\n", res);
        return;
    }

    switch ((status_t)(-res)){
        case STATUS_EOF:        printf("Error: input ended before the first value\n"); break;
        case STATUS_ERR_NULL:   printf("Error: NULL pointer\n"); break;
        case STATUS_ERR_FORMAT: printf("Error: unknown or unsupported specifier\n"); break;
        default:                printf("Error: unknown error\n"); break;
    }
}

#endif
