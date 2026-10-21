#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef enum {
    STATUS_OK,          // ok
    STATUS_ERR_NULL,    // NULL pointer
    STATUS_ERR_BASE,    // base not in [2..36]
    STATUS_ERR_CNT,     // cnt < 1
    STATUS_ERR_DIGIT,   // empty string or digit >= base
    STATUS_ERR_ALLOC    // malloc failed
} status_t;


// helpers
size_t len_s(const char* s){
    size_t n = 0;
    while (s[n] != '\0') n++;
    return n;
}

// char -> digit value
int digit(char c){
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

// digit value -> char (upper case)
char to_char(int d){
    if (d < 10) return (char)('0' + d);
    return (char)('A' + d - 10);
}

// check s
status_t check_num(const char* s, int base){
    if (s == NULL) return STATUS_ERR_NULL;
    if (s[0] == '\0') return STATUS_ERR_DIGIT;

    for (size_t i = 0; s[i] != '\0'; i++){
        int d = digit(s[i]);
        if (d < 0 || d >= base) return STATUS_ERR_DIGIT;
    }
    return STATUS_OK;
}

// remove leading zeros in place ("000" -> "0")
void trim_zeros(char* s){
    size_t k = 0;
    while (s[k] == '0' && s[k + 1] != '\0') k++;
    if (k == 0) return;

    size_t i = 0;
    while (s[i + k] != '\0'){
        s[i] = s[i + k];
        i++;
    }
    s[i] = '\0';
}


// column addition of two numbers in base (result must be freed)
status_t add_col(const char* a, const char* b, int base, char** res){
    if (a == NULL || b == NULL || res == NULL) return STATUS_ERR_NULL;
    if (base < 2 || base > 36) return STATUS_ERR_BASE;

    size_t la = len_s(a);
    size_t lb = len_s(b);
    size_t mx = (la > lb) ? la : lb;

    char* r = (char*)malloc(mx + 2);
    if (r == NULL) return STATUS_ERR_ALLOC;

    size_t i = la, j = lb, k = 0;
    int carry = 0;

    // digits are written from lowest to highest
    while (i > 0 || j > 0 || carry){
        int da = (i > 0) ? digit(a[--i]) : 0;
        int db = (j > 0) ? digit(b[--j]) : 0;
        int s = da + db + carry;
        r[k++] = to_char(s % base);
        carry = s / base;
    }
    if (k == 0) r[k++] = '0';
    r[k] = '\0';

    // reverse
    for (size_t l = 0, h = k - 1; l < h; l++, h--){
        char t = r[l];
        r[l] = r[h];
        r[h] = t;
    }

    trim_zeros(r);
    *res = r;
    return STATUS_OK;
}


// sum of cnt numbers in base (result must be freed)
status_t sum(char** res, int base, int cnt, ...){
    if (res == NULL) return STATUS_ERR_NULL;
    if (base < 2 || base > 36) return STATUS_ERR_BASE;
    if (cnt < 1) return STATUS_ERR_CNT;

    char* acc = (char*)malloc(2);
    if (acc == NULL) return STATUS_ERR_ALLOC;
    acc[0] = '0';
    acc[1] = '\0';

    va_list ap;
    va_start(ap, cnt);

    for (int i = 0; i < cnt; i++){
        const char* x = va_arg(ap, const char*);

        status_t s = check_num(x, base);
        if (s != STATUS_OK){
            va_end(ap);
            free(acc);
            return s;
        }

        char* tmp = NULL;
        s = add_col(acc, x, base, &tmp);
        if (s != STATUS_OK){
            va_end(ap);
            free(acc);
            return s;
        }

        free(acc);
        acc = tmp;
    }

    va_end(ap);
    *res = acc;
    return STATUS_OK;
}


// output
void print_sum(status_t s, int base, const char* res){
    switch (s){
        case STATUS_OK:        printf("Result (base %d): %s\n", base, res); break;
        case STATUS_ERR_NULL:  printf("Error: NULL pointer\n"); break;
        case STATUS_ERR_BASE:  printf("Error: base %d is not in [2..36]\n", base); break;
        case STATUS_ERR_CNT:   printf("Error: count of numbers must be >= 1\n"); break;
        case STATUS_ERR_DIGIT: printf("Error: invalid number for base %d\n", base); break;
        case STATUS_ERR_ALLOC: printf("Error: failed to allocate memory\n"); break;
        default:               printf("Error: unknown error\n"); break;
    }
}

#endif
