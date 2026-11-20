#ifndef FUNCTIONS1_H
#define FUNCTIONS1_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define MIN_R 1
#define MAX_R 5
#define MAX_DIGITS 34   // 32 bits (r = 1) + sign + '\0'

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,
    STATUS_ERR_RANGE_R,    // r is not in [1,5]
    STATUS_ERR_ALLOC
} status_t;

// n + 1 : flip trailing ones, then the first zero
unsigned int bit_inc(unsigned int n){
    unsigned int mask = 1;
    while (n & mask){
        n ^= mask;
        mask <<= 1;
    }
    n ^= mask;
    return n;
}

// n - 1 : flip trailing zeros, then the first one
unsigned int bit_dec(unsigned int n){
    unsigned int mask = 1u;
    while (!(n & mask)){
        n ^= mask;
        mask <<= 1;
    }
    n ^= mask;
    return n;
}

status_t decimal_to_base_2r(int n, int r, char** result){
    if (result == NULL)
        return STATUS_ERR_NULL_PTR;
    *result = NULL;
    if (r < MIN_R || r > MAX_R)
        return STATUS_ERR_RANGE_R;

    static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

    unsigned int mask = bit_dec(1u << r);   // 2^r - 1: r lowest bits set
    unsigned int num = (unsigned int)n;
    const int negative = (n < 0);
    if (negative)
        num = bit_inc(~num);                // |n| via two's complement, safe for INT_MIN

    char tmp[MAX_DIGITS];
    unsigned int len = 0;
    if (num == 0){
        tmp[len] = '0';
        len = bit_inc(len);
    }
    while (num > 0){
        tmp[len] = digits[num & mask];      // next digit = r lowest bits
        len = bit_inc(len);
        num >>= r;                          // divide by 2^r
    }
    if (negative){
        tmp[len] = '-';
        len = bit_inc(len);
    }

    char* res = (char*)malloc(bit_inc(len));
    if (res == NULL)
        return STATUS_ERR_ALLOC;

    // reversing
    for (unsigned int i = 0, j = len; i < len; i = bit_inc(i)){
        j = bit_dec(j);
        res[i] = tmp[j];
    }
    res[len] = '\0';

    *result = res;
    return STATUS_OK;
}

void print_base_2r(status_t state, int n, int r, const char* result){
    switch (state){
        case STATUS_ERR_NULL_PTR:
            printf("Error: null pointer\n");
            break;
        case STATUS_ERR_RANGE_R:
            printf("Error: r = %d is out of range [%d,%d]\n", r, MIN_R, MAX_R);
            break;
        case STATUS_ERR_ALLOC:
            printf("Error: failed to allocate memory\n");
            break;
        case STATUS_OK:
            printf("%d (r = %d, base %d): %s\n", n, r, 1 << r, result);
            break;
        default:
            printf("Other status\n");
            break;
    }
}

#endif
