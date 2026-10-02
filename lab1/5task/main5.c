#include <stdio.h>
#include "functions5.h"

#define X_VALUE 0.5L    // for row's summ

typedef status_t (*series_func_t)(const long double x, const long double eps, long double *result);
typedef status_t (*integral_func_t)(const long double eps, long double *result);

void print_hint(void)
{
    printf("usage: <eps>\n");
    printf("computes series for x = %.1Lf and integrals on [0, 1] with eps-acc\n",
           X_VALUE);
    printf("eps in [0.0000001, 0.1]\n\n");
}

void print_result(const char *name, const status_t status, const long double value)
{
    printf("  %s ", name);
    switch (status) {
        case STATUS_OK:              printf("%.9Lf\n", value); break;
        case STATUS_ERR_CONVERGENCE: printf("error: no convergence in max iterations\n"); break;
        case STATUS_ERR_OVERFLOW:    printf("error: overflow\n"); break;
        case STATUS_ERR_EPS_RANGE:   printf("error: eps out of range\n"); break;
        case STATUS_ERR_NULL_PTR:    printf("error: null pointer\n"); break;
        default:                     printf("error: unknown status\n"); break;
    }
}

int main(int argc, char *argv[])
{
    print_hint();

    if (argc != 2) {
        printf("error: expected 1 argument (eps), got %d\n", argc - 1);
        return 1;
    }

    long double eps = 0.0L;
    switch (parse_eps(argv[1], &eps)) {
        case STATUS_OK:
            break;
        case STATUS_ERR_EPS_RANGE:
            printf("error: eps must be in [0.0000001, 0.1]\n");
            return 1;
        case STATUS_ERR_OVERFLOW:
            printf("error: number is too large (overflow)\n");
            return 1;
        default:
            printf("error: <%s> is not a valid number\n", argv[1]);
            return 1;
    }

    const char *series_names[] = {"a) sum x^n / n!                        =",
                                  "b) sum (-1)^n x^2n / (2n)!             =",
                                  "c) sum 3^3n (n!)^3 x^2n / (3n)!        =",
                                  "d) sum (-1)^n (2n-1)!! x^2n / (2n)!!   ="};
    const series_func_t series[] = {sum_a, sum_b, sum_c, sum_d};

    const char *integral_names[] = {"a) int ln(1+x) / x    =",
                                    "b) int e^(-x^2 / 2)   =",
                                    "c) int ln(1 / (1-x))  =",
                                    "d) int x^x            ="};
    const integral_func_t integrals[] = {integral_a, integral_b, integral_c, integral_d};

    printf("eps = %.9Lf\n\nrow at x = %.1Lf:\n", eps, X_VALUE);
    for (int i = 0; i < 4; i++) {
        long double value = 0.0L;
        const status_t st = series[i](X_VALUE, eps, &value);
        print_result(series_names[i], st, value);
    }

    printf("\nintegrals on [0, 1]:\n");
    for (int i = 0; i < 4; i++) {
        long double value = 0.0L;
        const status_t st = integrals[i](eps, &value);
        print_result(integral_names[i], st, value);
    }
    return 0;
}