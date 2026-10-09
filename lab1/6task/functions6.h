#ifndef FUNCTIONS6_H
#define FUNCTIONS6_H

#include <stdarg.h>
#include <stddef.h>
#include <math.h>

#define MIN_VERTICES 3
#define MIN_BASE 2
#define MAX_BASE 36
#define MAX_KAPREKAR 4294967295ULL
#define MAX_ITER 1000

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,
    STATUS_ERR_COUNT,
    STATUS_ERR_EPS,
    STATUS_ERR_NOT_FINITE,
    STATUS_ERR_BASE,
    STATUS_ERR_NAN,
    STATUS_ERR_CAPACITY,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_DOMAIN,
    STATUS_ERR_INTERVAL,
    STATUS_ERR_NO_ROOT,
    STATUS_ERR_ITERATIONS
} status_t;

typedef double (*real_func_t)(const double x);

typedef struct {
    double x;
    double y;
} point_t;

status_t check_eps(const double eps)
{
    if (!isfinite(eps) || eps <= 0.0)
        return STATUS_ERR_EPS;
    return STATUS_OK;
}

// 6.1 - convex

status_t read_point(va_list *args, point_t *p)
{
    p->x = va_arg(*args, double);
    p->y = va_arg(*args, double);
    if (!isfinite(p->x) || !isfinite(p->y))
        return STATUS_ERR_NOT_FINITE;
    return STATUS_OK;
}

void mark_turn(const point_t a, const point_t b, const point_t c, const double eps,
               int *has_left, int *has_right)
{
    const double cross = (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
    if (cross > eps)
        *has_left = 1;
    else if (cross < -eps)
        *has_right = 1;
}

status_t is_convex(int *result, const double eps, const int n, ...)
{
    if (result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (check_eps(eps) != STATUS_OK)
        return STATUS_ERR_EPS;
    if (n < MIN_VERTICES)
        return STATUS_ERR_COUNT;

    va_list args;
    va_start(args, n);

    point_t first = {0.0, 0.0}, second = first, a = first, b = first, c = first;
    int has_left = 0, has_right = 0;
    for (int i = 0; i < n; i++) {
        if (read_point(&args, &c) != STATUS_OK) {
            va_end(args);
            return STATUS_ERR_NOT_FINITE;
        }
        if (i == 0)
            first = c;
        else if (i == 1)
            second = c;
        else
            mark_turn(a, b, c, eps, &has_left, &has_right);
        a = b;
        b = c;
    }
    va_end(args);

    mark_turn(a, b, first, eps, &has_left, &has_right);
    mark_turn(b, first, second, eps, &has_left, &has_right);

    *result = (has_left != has_right);
    return STATUS_OK;
}

// 6.2 - polynomial

status_t poly_value(double *result, const double x, const int n, ...)
{
    if (result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (!isfinite(x))
        return STATUS_ERR_NOT_FINITE;
    if (n < 0)
        return STATUS_ERR_COUNT;

    va_list args;
    va_start(args, n);

    double value = 0.0;
    for (int i = 0; i <= n; i++) {
        const double coef = va_arg(args, double);
        if (!isfinite(coef)) {
            va_end(args);
            return STATUS_ERR_NOT_FINITE;
        }
        value = value * x + coef;
        if (!isfinite(value)) {
            va_end(args);
            return STATUS_ERR_OVERFLOW;
        }
    }
    va_end(args);

    *result = value;
    return STATUS_OK;
}

// 6.3 - kaprekar

int digit_value(const char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    return -1;
}

status_t parse_in_base(const char *s, const int base, unsigned long long *n)
{
    if (s == NULL || n == NULL)
        return STATUS_ERR_NULL_PTR;
    if (base < MIN_BASE || base > MAX_BASE)
        return STATUS_ERR_BASE;
    if (*s == '\0')
        return STATUS_ERR_NAN;

    unsigned long long value = 0;
    for (; *s != '\0'; s++) {
        const int d = digit_value(*s);
        if (d < 0 || d >= base)
            return STATUS_ERR_NAN;
        if (value > (MAX_KAPREKAR - d) / base)
            return STATUS_ERR_OVERFLOW;
        value = value * base + d;
    }

    *n = value;
    return STATUS_OK;
}

int is_kaprekar(const unsigned long long n, const int base)
{
    if (n == 1)
        return 1;

    const unsigned long long sq = n * n;
    for (unsigned long long pw = base; pw <= sq; pw *= base) {
        const unsigned long long left = sq / pw;
        const unsigned long long right = sq % pw;
        if (right > 0 && left + right == n)
            return 1;
        if (pw > sq / base)
            break;
    }
    return 0;
}

status_t find_kaprekar(const char **found, int *found_count, const int capacity,
                       const int base, const int count, ...)
{
    if (found == NULL || found_count == NULL)
        return STATUS_ERR_NULL_PTR;
    if (base < MIN_BASE || base > MAX_BASE)
        return STATUS_ERR_BASE;
    if (count < 0)
        return STATUS_ERR_COUNT;
    if (capacity < count)
        return STATUS_ERR_CAPACITY;

    va_list args;
    va_start(args, count);

    int k = 0;
    for (int i = 0; i < count; i++) {
        const char *s = va_arg(args, const char *);
        unsigned long long n = 0;
        const status_t st = parse_in_base(s, base, &n);
        if (st != STATUS_OK) {
            va_end(args);
            return st;
        }
        if (is_kaprekar(n, base))
            found[k++] = s;
    }
    va_end(args);

    *found_count = k;
    return STATUS_OK;
}

// 6.4 - geometric mean

status_t geom_mean(double *result, const int count, ...)
{
    if (result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (count <= 0)
        return STATUS_ERR_COUNT;

    va_list args;
    va_start(args, count);

    double log_sum = 0.0;
    int has_zero = 0;
    for (int i = 0; i < count; i++) {
        const double x = va_arg(args, double);
        if (!isfinite(x)) {
            va_end(args);
            return STATUS_ERR_NOT_FINITE;
        }
        if (x < 0.0) {
            va_end(args);
            return STATUS_ERR_DOMAIN;
        }
        if (x == 0.0)
            has_zero = 1;
        else
            log_sum += log(x);
    }
    va_end(args);

    *result = has_zero ? 0.0 : exp(log_sum / count);
    return STATUS_OK;
}

// 6.5 - fast pow

double pow_rec(const double x, const unsigned long long n)
{
    if (n == 0)
        return 1.0;
    const double half = pow_rec(x, n / 2);
    return (n % 2 == 0) ? half * half : half * half * x;
}

status_t fast_pow(double *result, const double x, const int n, const double eps)
{
    if (result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (check_eps(eps) != STATUS_OK)
        return STATUS_ERR_EPS;
    if (!isfinite(x))
        return STATUS_ERR_NOT_FINITE;
    if (n < 0 && fabs(x) <= eps)
        return STATUS_ERR_DOMAIN;

    const long long power = n;
    double value = pow_rec(x, power < 0 ? -power : power);
    if (n < 0 && isfinite(value))
        value = 1.0 / value;
    if (!isfinite(value))
        return STATUS_ERR_OVERFLOW;

    *result = value;
    return STATUS_OK;
}

// 6.6 - dichotomy

status_t dichotomy(double *root, const double a, const double b, const double eps, const real_func_t f)
{
    if (root == NULL || f == NULL)
        return STATUS_ERR_NULL_PTR;
    if (check_eps(eps) != STATUS_OK)
        return STATUS_ERR_EPS;
    if (!isfinite(a) || !isfinite(b))
        return STATUS_ERR_NOT_FINITE;
    if (a >= b)
        return STATUS_ERR_INTERVAL;

    double lo = a, hi = b;
    double f_lo = f(lo);
    const double f_hi = f(hi);
    if (!isfinite(f_lo) || !isfinite(f_hi))
        return STATUS_ERR_DOMAIN;

    if (fabs(f_lo) < eps) { *root = lo; return STATUS_OK; }
    if (fabs(f_hi) < eps) { *root = hi; return STATUS_OK; }

    if ((f_lo < 0.0) == (f_hi < 0.0))
        return STATUS_ERR_NO_ROOT;

    for (int i = 0; i < MAX_ITER; i++) {
        const double mid = lo + (hi - lo) / 2.0;
        const double f_mid = f(mid);
        if (!isfinite(f_mid))
            return STATUS_ERR_DOMAIN;

        if ((hi - lo) / 2.0 < eps || fabs(f_mid) < eps) {
            *root = mid;
            return STATUS_OK;
        }

        if ((f_mid < 0.0) == (f_lo < 0.0)) {
            lo = mid;
            f_lo = f_mid;
        } else {
            hi = mid;
        }
    }
    return STATUS_ERR_ITERATIONS;
}

#endif
