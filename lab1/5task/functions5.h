#ifndef FUNCTIONS5_H
#define FUNCTIONS5_H

#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#define EPS_MIN 1e-7L
#define EPS_MAX 0.1L
#define MAX_ITER 100000000LL    // avoiding loop
#define MAX_PARTS (1LL << 30)   // razbieniyeeee

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,       // NULL
    STATUS_ERR_NAN,            // str is nan
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_EPS_RANGE,      // eps is not in [EPS_MIN, EPS_MAX]
    STATUS_ERR_CONVERGENCE     // didnt convergence
} status_t;

// T(n+1) / T(n)
typedef long double (*ratio_func_t)(const long double x, const long long n);
typedef long double (*real_func_t)(const long double x);


// parsing eps
status_t parse_real(const char *str, long double *result)
{
    if (str == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;

    size_t i = (str[0] == '-' || str[0] == '+') ? 1 : 0;
    int digits = 0, dots = 0;
    for (; str[i] != '\0'; i++) {
        if (isdigit((unsigned char)str[i]))
            digits++;
        else if (str[i] == '.' && dots == 0)
            dots++;
        else
            return STATUS_ERR_NAN;
    }
    if (digits == 0)
        return STATUS_ERR_NAN;

    char *end = NULL;
    const long double value = strtold(str, &end);
    if (end == str || *end != '\0')
        return STATUS_ERR_NAN;
    if (!isfinite(value))
        return STATUS_ERR_OVERFLOW;

    *result = value;
    return STATUS_OK;
}

status_t check_eps(const long double eps)
{
    if (!isfinite(eps) || eps < EPS_MIN || eps > EPS_MAX)
        return STATUS_ERR_EPS_RANGE;
    return STATUS_OK;
}

status_t parse_eps(const char *str, long double *eps)
{
    long double value = 0.0L;
    const status_t st = parse_real(str, &value);
    if (st != STATUS_OK)
        return st;
    if (check_eps(value) != STATUS_OK)
        return STATUS_ERR_EPS_RANGE;

    *eps = value;
    return STATUS_OK;
}

// rows

// start with first (номер n),
// get the next T by multiplying the previous one times ratio(x, n),
// stop when |T| <= eps
status_t sum_series(const long double x, const long double eps, const long double first,
                    const long long n_start, const ratio_func_t ratio, long double *result)
{
    if (ratio == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (check_eps(eps) != STATUS_OK)
        return STATUS_ERR_EPS_RANGE;

    long double term = first, sum = first;
    for (long long n = n_start; fabsl(term) > eps; n++) {
        if (n - n_start >= MAX_ITER)
            return STATUS_ERR_CONVERGENCE;
        term *= ratio(x, n);
        sum += term;
        if (!isfinite(sum))
            return STATUS_ERR_OVERFLOW;
    }
    *result = sum;
    return STATUS_OK;
}

// (a) T(n) = x^n / n!
long double ratio_a(const long double x, const long long n)
{
    return x / (n + 1);
}

// (b) T(n) = (-1)^n x^(2n) / (2n)!
long double ratio_b(const long double x, const long long n)
{
    return -x * x / ((2.0L * n + 1) * (2.0L * n + 2));
}

// (c) T(n) = 3^(3n) (n!)^3 x^(2n) / (3n)!
long double ratio_c(const long double x, const long long n)
{
    return 9.0L * (n + 1) * (n + 1) * x * x / ((3.0L * n + 1) * (3.0L * n + 2));
}

// (d) T(n) = (-1)^n (2n-1)!! x^(2n) / (2n)!!
long double ratio_d(const long double x, const long long n)
{
    return -(2.0L * n + 1) * x * x / (2.0L * n + 2);
}

status_t sum_a(const long double x, const long double eps, long double *result)
{
    return sum_series(x, eps, 1.0L, 0, ratio_a, result);            // с n = 0, T(0) = 1
}

status_t sum_b(const long double x, const long double eps, long double *result)
{
    return sum_series(x, eps, 1.0L, 0, ratio_b, result);            // с n = 0, T(0) = 1
}

status_t sum_c(const long double x, const long double eps, long double *result)
{
    return sum_series(x, eps, 1.0L, 0, ratio_c, result);            // с n = 0, T(0) = 1
}

status_t sum_d(const long double x, const long double eps, long double *result)
{
    return sum_series(x, eps, -x * x / 2.0L, 1, ratio_d, result);   // с n = 1, T(1) = -x^2/2
}

// integrals

// [0, 1] n parts
long double midpoint(const real_func_t f, const long long n)
{
    const long double h = 1.0L / n;
    long double sum = 0.0L;
    for (long long i = 0; i < n; i++)
        sum += f((i + 0.5L) * h);
    return sum * h;
}


status_t integrate(const real_func_t f, const long double eps, long double *result)
{
    if (f == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (check_eps(eps) != STATUS_OK)
        return STATUS_ERR_EPS_RANGE;

    long double prev = midpoint(f, 1);
    for (long long n = 2; n <= MAX_PARTS; n *= 2) {
        const long double cur = midpoint(f, n);
        if (!isfinite(cur))
            return STATUS_ERR_OVERFLOW;
        if (fabsl(cur - prev) <= eps) {
            *result = cur;
            return STATUS_OK;
        }
        prev = cur;
    }
    return STATUS_ERR_CONVERGENCE;
}

long double f_a(const long double x) { return logl(1.0L + x) / x; }
long double f_b(const long double x) { return expl(-x * x / 2.0L); }
long double f_c(const long double x) { return logl(1.0L / (1.0L - x)); }
long double f_d(const long double x) { return powl(x, x); }

status_t integral_a(const long double eps, long double *result) { return integrate(f_a, eps, result); }
status_t integral_b(const long double eps, long double *result) { return integrate(f_b, eps, result); }
status_t integral_c(const long double eps, long double *result) { return integrate(f_c, eps, result); }
status_t integral_d(const long double eps, long double *result) { return integrate(f_d, eps, result); }

#endif
