#ifndef FUNCTIONS3_H
#define FUNCTIONS3_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>

#define EPS_MIN 1e-9L
#define EPS_MAX 1.0L
#define COEF_COUNT 3
#define PERM_COUNT 6        // 3!

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,       // NULL
    STATUS_ERR_NAN,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_EPS_RANGE,
    STATUS_ERR_ZERO,
    STATUS_ERR_NOT_POSITIVE
} status_t;

typedef enum {
    ROOTS_TWO,        // 2 real roots
    ROOTS_ONE,        // 2 root (d = 0 or linear)
    ROOTS_NO_REAL,    // D < 0
    ROOTS_ANY,        // 0 = 0
    ROOTS_NONE        // c = 0, c != 0
} roots_kind_t;

typedef struct {
    roots_kind_t kind;
    long double x1;
    long double x2;
} roots_t;


// parsing input (float)
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

// parsing int
status_t parse_integer(const char *str, long long *result)
{
    if (str == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;

    const size_t len = strlen(str);
    if (len == 0)
        return STATUS_ERR_NAN;

    size_t i = (str[0] == '-' || str[0] == '+') ? 1 : 0;
    if (i == len)
        return STATUS_ERR_NAN;          // sign given only
    for (; i < len; i++)
        if (!isdigit((unsigned char)str[i]))
            return STATUS_ERR_NAN;

    char *end = NULL;
    const long long value = strtoll(str, &end, 10);
    if (end == str || *end != '\0')
        return STATUS_ERR_NAN;

	// due to overflow strtoll returned max and min
    if (value == LLONG_MAX || value == LLONG_MIN)
        return STATUS_ERR_OVERFLOW;

    *result = value;
    return STATUS_OK;
}



bool same_triple(const long double x[COEF_COUNT], const long double y[COEF_COUNT],
                 const long double eps)
{
    for (int i = 0; i < COEF_COUNT; i++)
        if (fabsl(x[i] - y[i]) > eps)
            return false;
    return true;
}


// -q
// 6 different (? checking via eps-comparison) equations
status_t unique_permutations(const long double eps, const long double coefs[COEF_COUNT],
                             long double perms[PERM_COUNT][COEF_COUNT], int *count)
{
    if (coefs == NULL || perms == NULL || count == NULL)
        return STATUS_ERR_NULL_PTR;
    if (check_eps(eps) != STATUS_OK)
        return STATUS_ERR_EPS_RANGE;

    const int order[PERM_COUNT][COEF_COUNT] = {
        {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
    };

    *count = 0;
    for (int p = 0; p < PERM_COUNT; p++) {
        long double candidate[COEF_COUNT];
        for (int i = 0; i < COEF_COUNT; i++)
            candidate[i] = coefs[order[p][i]];

        bool duplicate = false;
        for (int q = 0; q < *count && !duplicate; q++)
            duplicate = same_triple(perms[q], candidate, eps);

        if (!duplicate) {
            for (int i = 0; i < COEF_COUNT; i++)
                perms[*count][i] = candidate[i];
            (*count)++;
        }
    }
    return STATUS_OK;
}

// a = 0 -> linear
status_t solve_equation(const long double eps, const long double a, const long double b,
                        const long double c, roots_t *roots)
{
    if (roots == NULL)
        return STATUS_ERR_NULL_PTR;
    if (check_eps(eps) != STATUS_OK)
        return STATUS_ERR_EPS_RANGE;

    if (fabsl(a) <= eps) {
        if (fabsl(b) <= eps) {
            roots->kind = (fabsl(c) <= eps) ? ROOTS_ANY : ROOTS_NONE;
            return STATUS_OK;
        }
        const long double x = -c / b;
        if (!isfinite(x))
            return STATUS_ERR_OVERFLOW;
        roots->kind = ROOTS_ONE;
        roots->x1 = x;
        return STATUS_OK;
    }

    const long double d = b * b - 4.0L * a * c;
    if (!isfinite(d))
        return STATUS_ERR_OVERFLOW;

    if (d < -eps) {
        roots->kind = ROOTS_NO_REAL;
        return STATUS_OK;
    }
    if (d <= eps) {                            // |D| <= eps
        const long double x = -b / (2.0L * a);
        if (!isfinite(x))
            return STATUS_ERR_OVERFLOW;
        roots->kind = ROOTS_ONE;
        roots->x1 = x;
        return STATUS_OK;
    }

    const long double sq = sqrtl(d);
    const long double x1 = (-b - sq) / (2.0L * a);
    const long double x2 = (-b + sq) / (2.0L * a);
    if (!isfinite(x1) || !isfinite(x2))
        return STATUS_ERR_OVERFLOW;
    roots->kind = ROOTS_TWO;
    roots->x1 = x1;
    roots->x2 = x2;
    return STATUS_OK;
}

// -m

status_t check_multiple(const long long a, const long long b, bool *is_multiple)
{
    if (is_multiple == NULL)
        return STATUS_ERR_NULL_PTR;
    if (a == 0 || b == 0)
        return STATUS_ERR_ZERO;


    *is_multiple = (b == -1) || (a % b == 0);
    return STATUS_OK;
}

// -t

status_t check_right_triangle(const long double eps, const long double a, const long double b,
                              const long double c, bool *is_right)
{
    if (is_right == NULL)
        return STATUS_ERR_NULL_PTR;
    if (check_eps(eps) != STATUS_OK)
        return STATUS_ERR_EPS_RANGE;
    if (a <= eps || b <= eps || c <= eps)
        return STATUS_ERR_NOT_POSITIVE;

    const long double a2 = a * a, b2 = b * b, c2 = c * c;
    if (!isfinite(a2) || !isfinite(b2) || !isfinite(c2) || !isfinite(a2 + b2 + c2))
        return STATUS_ERR_OVERFLOW;


    *is_right = fabsl(a2 + b2 - c2) <= eps
             || fabsl(a2 + c2 - b2) <= eps
             || fabsl(b2 + c2 - a2) <= eps;
    return STATUS_OK;
}

#endif
