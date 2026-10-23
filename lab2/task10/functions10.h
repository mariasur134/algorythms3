#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef enum {
    STATUS_OK,          // ok
    STATUS_ERR_NULL,    // NULL pointer
    STATUS_ERR_DEGREE,  // n < 0
    STATUS_ERR_EPS,     // eps <= 0
    STATUS_ERR_ALLOC    // malloc failed
} status_t;


double abs_d(double x){
    return (x < 0) ? -x : x;
}

// poly value (horner) [coefficients array, power, point at which we evaluate, result]
status_t poly_value(const double* c, int n, double x, double* res){
    if (c == NULL || res == NULL) return STATUS_ERR_NULL;
    if (n < 0) return STATUS_ERR_DEGREE;

    double v = 0;
    for (int i = 0; i <= n; i++)
        v = v * x + c[i];

    *res = v;
    return STATUS_OK;
}


// [eps, a (for x-a), coefficients array, res, power, ...]
status_t decompose(double eps, double a, double** res, int n, ...){
    if (res == NULL) return STATUS_ERR_NULL;
    if (eps <= 0) return STATUS_ERR_EPS;
    if (n < 0) return STATUS_ERR_DEGREE;

    double* c = (double*)malloc(sizeof(double) * (size_t)(n + 1));
    if (c == NULL) return STATUS_ERR_ALLOC;

    va_list ap;
    va_start(ap, n);
    for (int i = 0; i <= n; i++)
        c[i] = va_arg(ap, double);
    va_end(ap);

    // i - coefficient position
    for (int k = 0; k < n; k++)
        for (int i = 1; i <= n - k; i++)
            c[i] += a * c[i - 1];

    // remove tiny values like 1e-16 / -0
    for (int i = 0; i <= n; i++)
        if (abs_d(c[i]) < eps) c[i] = 0;

    // changing value of res to adress of new array (c)
    *res = c;
    return STATUS_OK;
}


// output
void print_error(status_t s){
    switch (s){
        case STATUS_ERR_NULL:   printf("Error: NULL pointer\n"); break;
        case STATUS_ERR_DEGREE: printf("Error: degree must be >= 0\n"); break;
        case STATUS_ERR_EPS:    printf("Error: eps must be > 0\n"); break;
        case STATUS_ERR_ALLOC:  printf("Error: failed to allocate memory\n"); break;
        default:                printf("Error: unknown error\n"); break;
    }
}

// prints g and checks f(x) == g(x - a)
void print_result(status_t s, const double* f, const double* g, int n,
                  double a, double x, double eps){
    if (s != STATUS_OK){
        print_error(s);
        return;
    }


    printf("g: ");
    for (int i = 0; i <= n; i++)
        printf("%g ", g[i]);
    printf("\n");

    printf("g(x-a) = ");
    for (int i = 0; i <= n; i++){
        if (i > 0) printf(" + ");
        printf("%g", g[i]);
        if (n - i > 0)
            printf(" * (x - %g)^%d", a, n - i);
    }
    printf("\n");

    // checking
    double fv = 0, gv = 0;
    poly_value(f, n, x, &fv);
    poly_value(g, n, x - a, &gv);
    printf("check (at x = %g): f(x) = %g, g(x-a) = %g -> %s\n\n",
           x, fv, gv, (abs_d(fv - gv) < eps) ? "OK" : "FAIL");
}

#endif
