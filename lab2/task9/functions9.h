#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// denominator <= MAX_Q (up to 6 digits after .)
#ifndef EPS
#define EPS 1e-13
#endif

#define MAX_Q 1000000   // max denominator

typedef enum {
    STATUS_OK,          // ok
    STATUS_ERR_NULL,    // NULL pointer
    STATUS_ERR_BASE,    // base < 2
    STATUS_ERR_CNT,     // count < 1
    STATUS_ERR_RANGE,   // number not in (0; 1)
    STATUS_ERR_FRAC,    // cannot convert to fraction p/q
    STATUS_ERR_ALLOC    // malloc failed
} status_t;


double abs_d(double x){
    return (x < 0) ? -x : x;
}

// x -> p/q
status_t to_frac(double x, long long* p, long long* q){
    if (p == NULL || q == NULL) return STATUS_ERR_NULL;

    for (long long d = 1; d <= MAX_Q; d++){
        long long n = (long long)(x * d + 0.5);
        if (abs_d(x - (double)n / d) < EPS){
            *p = n;
            *q = d;
            return STATUS_OK;
        }
    }
    return STATUS_ERR_FRAC;
}

// p/q is finite in base <=> every prime divisor of q divides base
int is_finite(long long q, int base){
    for (long long d = 2; d * d <= q; d++){
        if (q % d == 0){
            if (base % d != 0) return 0;
            while (q % d == 0) q /= d;
        }
    }
    if (q > 1 && base % q != 0) return 0;
    return 1;
}

// varargs: cnt doubles in (0; 1)
// *res[i] = 1 - finite, 0 - infinite; must be freed
status_t check_fin(int** res, int base, int cnt, ...){
    if (res == NULL) return STATUS_ERR_NULL;
    if (base < 2) return STATUS_ERR_BASE;
    if (cnt < 1) return STATUS_ERR_CNT;

    int* r = (int*)malloc(sizeof(int) * (size_t)cnt);
    if (r == NULL) return STATUS_ERR_ALLOC;

    va_list ap;
    va_start(ap, cnt);

    for (int i = 0; i < cnt; i++){
        double x = va_arg(ap, double);

        // x != x is true only for NaN
        if (x != x || x < EPS || x > 1 - EPS){
            va_end(ap);
            free(r);
            return STATUS_ERR_RANGE;
        }

        long long p = 0, q = 1;
        status_t s = to_frac(x, &p, &q);
        if (s != STATUS_OK){
            va_end(ap);
            free(r);
            return s;
        }

        r[i] = is_finite(q, base);
    }

    va_end(ap);
    *res = r;
    return STATUS_OK;
}


// output
void print_error(status_t s){
    switch (s){
        case STATUS_ERR_NULL:  printf("Error: NULL pointer\n"); break;
        case STATUS_ERR_BASE:  printf("Error: base must be >= 2\n"); break;
        case STATUS_ERR_CNT:   printf("Error: count of numbers must be >= 1\n"); break;
        case STATUS_ERR_RANGE: printf("Error: number must be in (0; 1)\n"); break;
        case STATUS_ERR_FRAC:  printf("Error: cannot convert number to fraction (denominator > %d)\n", MAX_Q); break;
        case STATUS_ERR_ALLOC: printf("Error: failed to allocate memory\n"); break;
        default:               printf("Error: unknown error\n"); break;
    }
}

void print_res(status_t s, int base, const double* nums, const int* res, int cnt){
    if (s != STATUS_OK){
        print_error(s);
        return;
    }
    printf("base %d:\n", base);
    for (int i = 0; i < cnt; i++)
        printf("%g - %s%s\n", nums[i], res[i] ? "finite" : "infinite",
               (i < cnt - 1) ? "," : "");
}

#endif
