#ifndef FUNCTIONS1_H
#define FUNCTIONS1_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_X_H 100
#define MAX_X_E 10
#define MAX_HEX_DIG 16

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,       // null
    STATUS_ERR_NAN,            // str is not a number
    STATUS_ERR_OVERFLOW,       // overflow
    STATUS_ERR_RANGE_H,        // x is not in [1,100] (-h)
    STATUS_ERR_RANGE_E,        // x is not in [1,10] (-e)
    STATUS_ERR_RANGE_A,        // x < 1 (-a)
    STATUS_ERR_RANGE_F         // x < 0 (-f)
} status_t;

// parsing (+-int, by digit)
status_t parse_number(const char* str, long long* result){
    if (str == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;

    int neg = 0;
    if (*str == '+' || *str == '-'){
        neg = (*str == '-');
        str++;
    }
    if (*str == '\0')
        return STATUS_ERR_NAN;      // empty str or a sign only

    long long val = 0;
    for (; *str != '\0'; str++){
        if (!isdigit((unsigned char)*str))
            return STATUS_ERR_NAN;
        int d = *str - '0';
        if (val > (LLONG_MAX - d) / 10)
            return STATUS_ERR_OVERFLOW;
        val = val * 10 + d;
    }

    *result = neg ? -val : val;
    return STATUS_OK; //all good
}

// -h (nums in [1:100] devidable by x)
status_t multiples_in_range(long long x, long long* arr, int* size){
    if (arr == NULL || size == NULL)
        return STATUS_ERR_NULL_PTR;
    if (x < 1 || x > MAX_X_H)
        return STATUS_ERR_RANGE_H;

    int n = 0;
    for (long long i = x; i <= MAX_X_H; i += x)
        arr[n++] = i;

    *size = n;
    return STATUS_OK;
}

void print_multiples_in_range(status_t state, long long x, const long long* arr, int size){
    switch (state){
        case STATUS_ERR_NULL_PTR:
            printf("Error: null pointer\n");
            break;
        case STATUS_ERR_RANGE_H:
            printf("Error: number <%lld> is out of range [1,%d]\n", x, MAX_X_H);
            break;
        case STATUS_OK:
            if (arr == NULL){
                printf("Error: null pointer\n");
                break;
            }
            printf("Numbers from 1 to %d dividable by %lld: \n", MAX_X_H, x);
            for (int i = 0; i < size; i++)
                printf("%lld ", arr[i]);
            printf("\n");
            break;
        default:
            printf("Other status\n");
            break;
    }
}


// -p
status_t check_prime(long long x, int* is_prime){
    if (is_prime == NULL)
        return STATUS_ERR_NULL_PTR;

    *is_prime = 0;
    if (x < 2)
        return STATUS_OK;

    for (long long i = 2; i <= x / i; i++){
        if (x % i == 0)
            return STATUS_OK;
    }

    *is_prime = 1;
    return STATUS_OK;
}

void print_prime(status_t state, long long x, int is_prime){
    switch (state){
        case STATUS_ERR_NULL_PTR:
            printf("Error: null pointer\n");
            break;
        case STATUS_OK:
            if (is_prime)
                printf("%lld is prime\n", x);
            else
                printf("%lld is not prime\n", x);
            break;
        default:
            printf("Other status\n");
            break;
    }
}


// -s ()
status_t split_hex(long long x, int* digits, int* count){
    if (digits == NULL || count == NULL)
        return STATUS_ERR_NULL_PTR;

    // avoiding overflow LLONG_MIN
    unsigned long long n = (x >= 0) ? (unsigned long long)x : 0ULL - (unsigned long long)x;

    int tmp[MAX_HEX_DIG];
    int k = 0;
    if (n == 0)
        tmp[k++] = 0;
    while (n > 0){
        tmp[k++] = (int)(n % 16);
        n /= 16;
    }

    for (int i = 0; i < k; i++)
        digits[i] = tmp[k - 1 - i];

    *count = k;
    return STATUS_OK;
}

void print_hex(status_t state, long long x, const int* digits, int count){
    switch (state){
        case STATUS_ERR_NULL_PTR:
            printf("Error: null pointer\n");
            break;
        case STATUS_OK:
            if (digits == NULL){
                printf("Error: null pointer\n");
                break;
            }
            printf("Hex digits of %lld: ", x);
            for (int i = 0; i < count; i++){
                if (digits[i] < 10)
                    printf("%d ", digits[i]);
                else
                    printf("%c ", 'A' + (digits[i] - 10));
            }
            printf("\n");
            break;
        default:
            printf("Other status\n");
            break;
    }
}


// -e (table[base-1][exp -1] = base ^ exp)
status_t powers_table(long long x, long long table[][MAX_X_E], int* rows, int* cols){
    if (table == NULL || rows == NULL || cols == NULL)
        return STATUS_ERR_NULL_PTR;
    if (x < 1 || x > MAX_X_E)
        return STATUS_ERR_RANGE_E;

    for (int base = 1; base <= 10; base++){
        long long value = 1;
        for (int e = 1; e <= x; e++){
            value *= base;
            table[base - 1][e - 1] = value;
        }
    }
    *rows = 10;
    *cols = (int)x;
    return STATUS_OK;
}

void print_powers_table(status_t state, long long x, long long table[][MAX_X_E], int rows, int cols){
    switch (state){
        case STATUS_ERR_NULL_PTR:
            printf("Error: null pointer\n");
            break;
        case STATUS_ERR_RANGE_E:
            printf("Error: number <%lld> is out of range [1,%d]\n", x, MAX_X_E);
            break;
        case STATUS_OK:
            if (table == NULL){
                printf("Error: null pointer\n");
                break;
            }
            printf("Table of powers, exponents 1-%lld:\n", x);
            for (int base = 0; base < rows; base++){
                printf("Base %d: ", base + 1);
                for (int e = 0; e < cols; e++)
                    printf("%d^%d=%lld ", base + 1, e + 1, table[base][e]);
                printf("\n");
            }
            break;
        default:
            printf("Other status\n");
            break;
    }
}


// -a

// x*(x+1)/2: делим чётный множитель на 2 до умножения, чтобы не терять диапазон
status_t sum_natural(long long x, long long* result){
    if (result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (x < 1)
        return STATUS_ERR_RANGE_A;

    long long p, q;
    if (x % 2 == 0){
        p = x / 2;
        q = x + 1;
    } else {
        p = x;
        q = x / 2 + 1;
    }

    if (p > LLONG_MAX / q)
        return STATUS_ERR_OVERFLOW;

    *result = p * q;
    return STATUS_OK;
}

void print_sum(status_t state, long long x, long long result){
    switch (state){
        case STATUS_ERR_NULL_PTR:
            printf("Error: null pointer\n");
            break;
        case STATUS_ERR_RANGE_A:
            printf("Error: number <%lld> must be positive\n", x);
            break;
        case STATUS_ERR_OVERFLOW:
            printf("Error: sum overflow for number <%lld>\n", x);
            break;
        case STATUS_OK:
            printf("Sum of natural numbers 1..%lld = %lld\n", x, result);
            break;
        default:
            printf("Other status\n");
            break;
    }
}


// -f
status_t factorial_iter(long long x, long long* result){
    if (result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (x < 0)
        return STATUS_ERR_RANGE_F;

    long long value = 1;
    for (long long i = 2; i <= x; i++){
        if (value > LLONG_MAX / i)
            return STATUS_ERR_OVERFLOW;
        value *= i;
    }

    *result = value;
    return STATUS_OK;
}

void print_factorial(status_t state, long long x, long long result){
    switch (state){
        case STATUS_ERR_NULL_PTR:
            printf("Error: null pointer\n");
            break;
        case STATUS_ERR_RANGE_F:
            printf("Error: number <%lld> must be non-negative\n", x);
            break;
        case STATUS_ERR_OVERFLOW:
            printf("Error: factorial overflow for number <%lld>\n", x);
            break;
        case STATUS_OK:
            printf("Factorial of %lld is %lld\n", x, result);
            break;
        default:
            printf("Other status\n");
            break;
    }
}

#endif