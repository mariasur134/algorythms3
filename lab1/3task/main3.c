#include <stdio.h>
#include "functions3.h"

void print_hint(void)
{
    printf("Usage:\n");
    printf("  -q <eps> <a> <b> <c>  solve a*x^2 + b*x + c = 0\n");
    printf("  -m <a> <b>            check if a is a multiple of b\n");
    printf("  -t <eps> <a> <b> <c>  check if a, b, c can from a triangle\n");
}

void print_status_error(const status_t status, const char *arg)
{
    switch (status) {
        case STATUS_ERR_NAN:          printf("Error: <%s> is invalid number\n", arg); break;
        case STATUS_ERR_OVERFLOW:     printf("Error: number is too large (overflow)\n"); break;
        case STATUS_ERR_EPS_RANGE:    printf("Error: eps <%s> must be in [1e-9, 1]\n", arg); break;
        case STATUS_ERR_ZERO:         printf("Error: numbers must be >= 0\n"); break;
        case STATUS_ERR_NOT_POSITIVE: printf("Error: sides must be >0 \n"); break;
        case STATUS_ERR_NULL_PTR:     printf("Error: null pointer\n"); break;
        default:                      printf("Error: unknown status\n"); break;
    }
}

int parse_flag(const char *str, char *flag)
{
    if (strlen(str) != 2 || (str[0] != '-' && str[0] != '/'))
        return 0;
    if (str[1] != 'q' && str[1] != 'm' && str[1] != 't')
        return 0;
    *flag = str[1];
    return 1;
}

long double no_neg_zero(const long double x) { return x + 0.0L; }

void print_equation(const long double coefs[COEF_COUNT])
{
    printf("[%.9Lgx^2 %+.9Lgx %+.9Lg = 0]", coefs[0], coefs[1], coefs[2]);
}

void print_roots(const roots_t *roots)
{
    switch (roots->kind) {
        case ROOTS_TWO:
            printf("solution: [%.9Lf, %.9Lf]\n", no_neg_zero(roots->x1), no_neg_zero(roots->x2));
            break;
        case ROOTS_ONE:
            printf("solution: [%.9Lf]\n", no_neg_zero(roots->x1));
            break;
        case ROOTS_NO_REAL:
            printf("solution: [no real roots]\n");
            break;
        case ROOTS_ANY:
            printf("solution: [any x]\n");
            break;
        case ROOTS_NONE:
            printf("solution: [no solutions]\n");
            break;
    }
}


int parse_eps_and_three(char *argv[], long double *eps, long double values[COEF_COUNT])
{
    status_t st = parse_eps(argv[2], eps);
    if (st != STATUS_OK) {
        print_status_error(st, argv[2]);
        return 0;
    }
    for (int i = 0; i < COEF_COUNT; i++) {
        st = parse_real(argv[3 + i], &values[i]);
        if (st != STATUS_OK) {
            print_status_error(st, argv[3 + i]);
            return 0;
        }
    }
    return 1;
}

int run_q(char *argv[])
{
    long double eps = 0.0L, coefs[COEF_COUNT];
    if (!parse_eps_and_three(argv, &eps, coefs))
        return 1;

    long double perms[PERM_COUNT][COEF_COUNT];
    int count = 0;
    status_t st = unique_permutations(eps, coefs, perms, &count);
    if (st != STATUS_OK) {
        print_status_error(st, "");
        return 1;
    }

    for (int i = 0; i < count; i++) {
        roots_t roots;
        st = solve_equation(eps, perms[i][0], perms[i][1], perms[i][2], &roots);
        print_equation(perms[i]);
        printf(", ");
        if (st == STATUS_OK)
            print_roots(&roots);
        else
            print_status_error(st, "");
    }
    return 0;
}

int run_m(char *argv[])
{
    long long a = 0, b = 0;
    status_t st = parse_integer(argv[2], &a);
    if (st != STATUS_OK) {
        print_status_error(st, argv[2]);
        return 1;
    }
    st = parse_integer(argv[3], &b);
    if (st != STATUS_OK) {
        print_status_error(st, argv[3]);
        return 1;
    }

    bool is_multiple = false;
    st = check_multiple(a, b, &is_multiple);
    if (st != STATUS_OK) {
        print_status_error(st, "");
        return 1;
    }
    printf("%lld %s a multiple of %lld\n", a, is_multiple ? "is" : "is not", b);
    return 0;
}

int run_t(char *argv[])
{
    long double eps = 0.0L, sides[COEF_COUNT];
    if (!parse_eps_and_three(argv, &eps, sides))
        return 1;

    bool is_right = false;
    const status_t st = check_right_triangle(eps, sides[0], sides[1], sides[2], &is_right);
    if (st != STATUS_OK) {
        print_status_error(st, "");
        return 1;
    }
    printf("%.9Lg, %.9Lg, %.9Lg %s be sides of a right triangle\n",
           sides[0], sides[1], sides[2], is_right ? "can" : "can not");
    return 0;
}

int main(int argc, char *argv[])
{
    print_hint();

    if (argc < 2) {
        printf("Error: no flag given\n");
        return 1;
    }

    char flag = 0;
    if (!parse_flag(argv[1], &flag)) {
        printf("Error: unknown flag <%s>\n", argv[1]);
        return 1;
    }

    const int need_argc = (flag == 'm') ? 4 : 6;
    if (argc != need_argc) {
        printf("Error: flag -%c needs %d parameters, got %d\n", flag, need_argc - 2, argc - 2);
        return 1;
    }

    switch (flag) {
        case 'q': return run_q(argv);
        case 'm': return run_m(argv);
        case 't': return run_t(argv);
        default:  return 1;
    }
}
