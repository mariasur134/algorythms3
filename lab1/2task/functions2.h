#ifndef FUNCTIONS2_H
#define FUNCTIONS2_H

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define EPS_MIN 1e-4L
#define EPS_MAX 1e-1L
#define MAX_ITER 100000000LL    // avoiding endless cycle
#define NEWTON_MAX_ITER 1000

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,       // null
    STATUS_ERR_INPUT,          // str not a num
    STATUS_ERR_RANGE,          // eps ins not in range
    STATUS_ERR_CONVERGENCE,    // didnt converge
    STATUS_ERR_MEMORY          // couldnt allocate memory
} status_t;

typedef status_t (*calc_func_t)(const long double epsilon, long double *result);
typedef long double (*real_func_t)(const long double x);


status_t check_args(const long double epsilon, const long double *result)
{
    if (result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (!isfinite(epsilon) || epsilon < EPS_MIN || epsilon > EPS_MAX)
        return STATUS_ERR_RANGE;
    return STATUS_OK;
}

// limits: n = 2 / eps.
long long limit_steps(const long double epsilon) return (long long)ceill(2.0L / epsilon);

long double fast_pow(long double base, long long power){
    long double result = 1.0L;
    while (power > 0) {
        if (power % 2 == 1)
            result *= base;
        base *= base;
        power /= 2;
    }
    return result;
}

// newton for f(x) = 0
status_t newton(const real_func_t f, const real_func_t df, long double x,
                const long double epsilon, long double *result)
{
    for (int i = 0; i < NEWTON_MAX_ITER; i++) {
        const long double d = df(x);
        if (fabsl(d) <= epsilon)
            return STATUS_ERR_CONVERGENCE;
        const long double next = x - f(x) / d;
        if (!isfinite(next))
            return STATUS_ERR_CONVERGENCE;
        if (fabsl(next - x) <= epsilon) {
            *result = next;
            return STATUS_OK;
        }
        x = next;
    }
    return STATUS_ERR_CONVERGENCE;
}


// parsing input
status_t parse_epsilon(const char *str, long double *epsilon)
{
    if (str == NULL || epsilon == NULL)
        return STATUS_ERR_NULL_PTR;
    if (str[0] == '\0')
        return STATUS_ERR_INPUT;

    char *end = NULL;
    const long double value = strtold(str, &end);
    if (end == str || *end != '\0' || !isfinite(value))
        return STATUS_ERR_INPUT;
    if (value < EPS_MIN || value > EPS_MAX)
        return STATUS_ERR_RANGE;

    *epsilon = value;
    return STATUS_OK;
}

// e
status_t e_limit(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    const long long n = limit_steps(epsilon);
    *result = fast_pow(1.0L + 1.0L / n, n);
    return STATUS_OK;
}

// stop when 1/n! <= eps
status_t e_series(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long double sum = 1.0L, term = 1.0L;
    for (long long n = 1; n <= MAX_ITER; n++) {
        term /= n;
        sum += term;
        if (term <= epsilon) {
            *result = sum;
            return STATUS_OK;
        }
    }
    return STATUS_ERR_CONVERGENCE;
}

long double e_f(const long double x)  { return logl(x) - 1.0L; }   // ln x = 1
long double e_df(const long double x) { return 1.0L / x; }

status_t e_equation(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    return st != STATUS_OK ? st : newton(e_f, e_df, 2.0L, epsilon, result);
}

// pi

// a(n) [a(k) / a(k-1) = 4k(k-1) / (2k-1)^2]
status_t pi_limit(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    const long long n = limit_steps(epsilon);
    long double a = 4.0L;   // a(1)
    for (long long k = 2; k <= n; k++)
        a *= 4.0L * k * (k - 1) / ((2.0L * k - 1) * (2.0L * k - 1));

    *result = a;
    return STATUS_OK;
}

status_t pi_series(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long double sum = 0.0L;
    for (long long n = 1; n <= MAX_ITER; n++) {
        const long double term = 4.0L / (2.0L * n - 1.0L);
        sum += (n % 2 == 1) ? term : -term;
        if (term <= epsilon) {
            *result = sum;
            return STATUS_OK;
        }
    }
    return STATUS_ERR_CONVERGENCE;
}

long double pi_f(const long double x)  { return cosl(x / 2.0L); }
long double pi_df(const long double x) { return -sinl(x / 2.0L) / 2.0L; }

status_t pi_equation(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    return st != STATUS_OK ? st : newton(pi_f, pi_df, 3.0L, epsilon, result);
}

// ln2
status_t ln2_limit(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    const long long n = limit_steps(epsilon);
    *result = n * (powl(2.0L, 1.0L / n) - 1.0L);
    return STATUS_OK;
}

status_t ln2_series(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long double sum = 0.0L;
    for (long long n = 1; n <= MAX_ITER; n++) {
        const long double term = 1.0L / n;
        sum += (n % 2 == 1) ? term : -term;
        if (term <= epsilon) {
            *result = sum;
            return STATUS_OK;
        }
    }
    return STATUS_ERR_CONVERGENCE;
}

long double ln2_f(const long double x)  { return expl(x) - 2.0L; }  // e^x = 2
long double ln2_df(const long double x) { return expl(x); }

status_t ln2_equation(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    return st != STATUS_OK ? st : newton(ln2_f, ln2_df, 0.5L, epsilon, result);
}

// sqrt 2

status_t sqrt2_limit(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long double x = -0.5L;
    for (long long i = 0; i < MAX_ITER; i++) {
        const long double next = x - x * x / 2.0L + 1.0L;
        if (fabsl(next - x) <= epsilon) {
            *result = next;
            return STATUS_OK;
        }
        x = next;
    }
    return STATUS_ERR_CONVERGENCE;
}

status_t sqrt2_series(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long double product = 1.0L;
    for (long long k = 2; k <= 64; k++) {
        const long double factor = powl(2.0L, powl(2.0L, (long double)-k));
        product *= factor;
        if (product * (factor - 1.0L) <= epsilon) {
            *result = product;
            return STATUS_OK;
        }
    }
    return STATUS_ERR_CONVERGENCE;
}

long double sqrt2_f(const long double x)  { return x * x - 2.0L; }  // x^2 = 2
long double sqrt2_df(const long double x) { return 2.0L * x; }

status_t sqrt2_equation(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    return st != STATUS_OK ? st : newton(sqrt2_f, sqrt2_df, 1.0L, epsilon, result);
}

// gamma

// gamma = lim (H_n - ln n)
status_t gamma_limit(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    const long long n = limit_steps(epsilon);
    long double harmonic = 0.0L;
    for (long long k = 1; k <= n; k++)
        harmonic += 1.0L / k;

    *result = harmonic - logl((long double)n);
    return STATUS_OK;
}

status_t gamma_series(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    long double sum = -powl(acosl(-1.0L), 2.0L) / 6.0L;
    long long k = 2;
    for (long long r = 1; k <= MAX_ITER; r++) {
        long double block = 0.0L;
        const long long end = (r + 1) * (r + 1);
        for (; k < end; k++)
            block += 1.0L / (r * r) - 1.0L / k;
        sum += block;
        if (block <= epsilon) {
            *result = sum + 2.0L / (r + 1);
            return STATUS_OK;
        }
    }
    return STATUS_ERR_CONVERGENCE;
}

long double gamma_eq_f(const long double x, const long double limit)  { return expl(-x) - limit; }
long double gamma_eq_df(const long double x)                          { return -expl(-x); }

status_t gamma_equation(const long double epsilon, long double *result)
{
    const status_t st = check_args(epsilon, result);
    if (st != STATUS_OK)
        return st;

    const long long t = (long long)ceill(1.0L / (epsilon * epsilon));

    unsigned char *composite = malloc((size_t)(t / 8 + 1));
    if (composite == NULL)
        return STATUS_ERR_MEMORY;
    memset(composite, 0, (size_t)(t / 8 + 1));

    long double log_product = 0.0L;
    for (long long i = 2; i <= t; i++) {
        if (composite[i / 8] & (1 << (i % 8)))
            continue;
        log_product += log1pl(-1.0L / i);
        if (i > t / i)
            continue;
        for (long long j = i * i; j <= t; j += i)
            composite[j / 8] |= (unsigned char)(1 << (j % 8));
    }
    free(composite);

    const long double limit = expl(logl(logl((long double)t)) + log_product);

    long double x = 0.5L;
    for (int i = 0; i < NEWTON_MAX_ITER; i++) {
        const long double next = x - gamma_eq_f(x, limit) / gamma_eq_df(x);
        if (!isfinite(next))
            return STATUS_ERR_CONVERGENCE;
        if (fabsl(next - x) <= epsilon) {
            *result = next;
            return STATUS_OK;
        }
        x = next;
    }
    return STATUS_ERR_CONVERGENCE;
}

#endif