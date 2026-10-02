#ifndef FUNCTIONS8_H
#define FUNCTIONS8_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define START_CAP 16     // buf size
#define MIN_BASE 2
#define MAX_BASE 36

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,    // NULL
    STATUS_ERR_ALLOC,
    STATUS_ERR_LEXEME,      // nan
    STATUS_ERR_OVERFLOW,    // overflow
    STATUS_END
} status_t;


int digit_value(const char c){
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    return -1;
}

int is_space(const int c){
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

void skip_spaces(FILE *in){
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (!is_space(c)) {
            ungetc(c, in);
            return;
        }
    }
}

status_t read_lexeme(FILE *in, char **lexeme){
    if (in == NULL || lexeme == NULL)
        return STATUS_ERR_NULL_PTR;

    skip_spaces(in);

    int c = fgetc(in);
    if (c == EOF)
        return STATUS_END;

    size_t cap = START_CAP, len = 0;
    char *buf = (char *)malloc(cap);
    if (buf == NULL)
        return STATUS_ERR_ALLOC;

    while (c != EOF && !is_space(c)) {
        if (len + 1 >= cap) {                 // +1 is for'\0'
            cap *= 2;
            char *tmp = (char *)realloc(buf, cap);
            if (tmp == NULL) {
                free(buf);
                return STATUS_ERR_ALLOC;
            }
            buf = tmp;
        }
        buf[len++] = (char)c;
        c = fgetc(in);
    }
    buf[len] = '\0';

    *lexeme = buf;
    return STATUS_OK;
}

status_t analyze_lexeme(const char *lexeme, int *negative, const char **digits, int *min_base)
{
    if (lexeme == NULL || negative == NULL || digits == NULL || min_base == NULL)
        return STATUS_ERR_NULL_PTR;

    const char *p = lexeme;
    int neg = 0;
    if (*p == '-' || *p == '+') {
        neg = (*p == '-');
        p++;
    }
    if (*p == '\0')
        return STATUS_ERR_LEXEME;

    while (*p == '0' && p[1] != '\0')
        p++;

    int max_digit = 0;
    for (const char *q = p; *q != '\0'; q++) {
        const int d = digit_value(*q);
        if (d < 0)
            return STATUS_ERR_LEXEME;
        if (d > max_digit)
            max_digit = d;
    }

    *negative = neg;
    *digits = p;
    *min_base = (max_digit + 1 < MIN_BASE) ? MIN_BASE : max_digit + 1;
    return STATUS_OK;
}


status_t to_decimal(const char *digits, const int base, const int negative, long long *result)
{
    if (digits == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;

    const unsigned long long limit = negative ? (unsigned long long)LLONG_MAX + 1ULL
                                              : (unsigned long long)LLONG_MAX;
    unsigned long long value = 0;
    for (; *digits != '\0'; digits++) {
        const unsigned long long d = (unsigned long long)digit_value(*digits);
        if (value > (limit - d) / (unsigned long long)base)
            return STATUS_ERR_OVERFLOW;
        value = value * (unsigned long long)base + d;
    }

    if (!negative)
        *result = (long long)value;
    else if (value == (unsigned long long)LLONG_MAX + 1ULL)
        *result = LLONG_MIN;
    else
        *result = -(long long)value;
    return STATUS_OK;
}

status_t convert_lexeme(const char *lexeme, int *negative, const char **digits, int *base, long long *value){
    if (lexeme == NULL || negative == NULL || digits == NULL || base == NULL || value == NULL)
        return STATUS_ERR_NULL_PTR;

    const status_t st = analyze_lexeme(lexeme, negative, digits, base);
    if (st != STATUS_OK) return st;

    const status_t st_dec = to_decimal(*digits, *base, *negative, value);
    if (st_dec != STATUS_OK)
        return st_dec;

    if (*value == 0)
        *negative = 0;
    return STATUS_OK;
}


void print_number(FILE *out, const int negative, const char *digits, const int base, const long long value){
    fprintf(out, "%s%s base %d decimal %lld\n", negative ? "-" : "", digits, base, value);
}


status_t process_file(FILE *in, FILE *out){
    if (in == NULL || out == NULL)
        return STATUS_ERR_NULL_PTR;

    while (1) {
        char *lexeme = NULL;
        const status_t st = read_lexeme(in, &lexeme);
        if (st == STATUS_END)
            return STATUS_OK;
        if (st != STATUS_OK)
            return st;

        int negative = 0, base = 0;
        const char *digits = NULL;
        long long value = 0;

        switch (convert_lexeme(lexeme, &negative, &digits, &base, &value)) {
            case STATUS_OK:
                print_number(out, negative, digits, base, value);
                break;
            case STATUS_ERR_LEXEME:
                printf("invalid lexeme\n");
                break;
            case STATUS_ERR_OVERFLOW:
                printf("overflow\n");
                break;
            default:
                free(lexeme);
                return STATUS_ERR_NULL_PTR;
        }
        free(lexeme);
    }
}

#endif
