#include <stdio.h>
#include "functions6.h"

void print_status(const status_t status)
{
    switch (status) {
        case STATUS_OK:             printf("ok\n"); break;
        case STATUS_ERR_NULL_PTR:   printf("error: null pointer\n"); break;
        case STATUS_ERR_COUNT:      printf("error: bad count\n"); break;
        case STATUS_ERR_EPS:        printf("error: bad eps\n"); break;
        case STATUS_ERR_NOT_FINITE: printf("error: inf or nan\n"); break;
        case STATUS_ERR_BASE:       printf("error: bad base\n"); break;
        case STATUS_ERR_NAN:        printf("error: not a number\n"); break;
        case STATUS_ERR_CAPACITY:   printf("error: array too small\n"); break;
        case STATUS_ERR_OVERFLOW:   printf("error: overflow\n"); break;
        case STATUS_ERR_DOMAIN:     printf("error: bad value\n"); break;
        case STATUS_ERR_INTERVAL:   printf("error: a must be < b\n"); break;
        case STATUS_ERR_NO_ROOT:    printf("error: no root here\n"); break;
        case STATUS_ERR_ITERATIONS: printf("error: too many steps\n"); break;
        default:                    printf("error: unknown\n"); break;
    }
}

int report(const status_t status)
{
    if (status == STATUS_OK)
        return 1;
    print_status(status);
    return 0;
}

double f_sqrt2(const double x) { return x * x - 2.0; }
double f_cos(const double x)   { return cos(x) - x; }
double f_cube(const double x)  { return x * x * x - x - 1.0; }

// 6.1 - convex

void demo_convex(void)
{
    const double eps = 1e-9;
    int convex = 0;

    printf("6.1 convex\n");
    if (report(is_convex(&convex, eps, 4, 0.0, 0.0, 2.0, 0.0, 2.0, 2.0, 0.0, 2.0)))
        printf("square: %s\n", convex ? "yes" : "no");
    if (report(is_convex(&convex, eps, 3, 0.0, 0.0, 4.0, 0.0, 0.0, 3.0)))
        printf("triangle: %s\n", convex ? "yes" : "no");
    if (report(is_convex(&convex, eps, 5, 0.0, 0.0, 2.0, 0.0, 1.0, 1.0, 2.0, 2.0, 0.0, 2.0)))
        printf("arrow: %s\n", convex ? "yes" : "no");
    if (report(is_convex(&convex, eps, 4, 0.0, 0.0, 1.0, 0.0, 2.0, 0.0, 1.0, 2.0)))
        printf("point on edge: %s\n", convex ? "yes" : "no");
    printf("\n");
}

// 6.2 - polynomial

void demo_poly(void)
{
    double value = 0.0;

    printf("6.2 polynomial\n");
    if (report(poly_value(&value, 2.0, 2, 2.0, -3.0, 1.0)))
        printf("2x^2 - 3x + 1, x = 2: %g\n", value);
    if (report(poly_value(&value, 2.0, 3, 5.0, 4.0, 3.0, 2.0)))
        printf("5x^3 + 4x^2 + 3x + 2, x = 2: %g\n", value);
    if (report(poly_value(&value, -1.5, 0, 7.0)))
        printf("7, x = -1.5: %g\n", value);
    printf("\n");
}

// 6.3 - kaprekar

void print_found(const char *title, const char **found, const int found_count)
{
    printf("%s:", title);
    if (found_count == 0)
        printf(" none");
    for (int i = 0; i < found_count; i++)
        printf(" %s", found[i]);
    printf("\n");
}

void demo_kaprekar(void)
{
    const char *found[10];
    int found_count = 0;

    printf("6.3 kaprekar\n");
    if (report(find_kaprekar(found, &found_count, 10, 10, 10,
                             "1", "9", "10", "45", "55", "7", "297", "100", "4879", "5292")))
        print_found("base 10", found, found_count);
    if (report(find_kaprekar(found, &found_count, 10, 2, 5, "1", "10", "11", "110", "111")))
        print_found("base 2", found, found_count);
    if (report(find_kaprekar(found, &found_count, 10, 16, 4, "F", "33", "a", "5B")))
        print_found("base 16", found, found_count);
    printf("\n");
}

// 6.4 - geometric mean

void demo_geom(void)
{
    double value = 0.0;

    printf("6.4 geometric mean\n");
    if (report(geom_mean(&value, 4, 1.0, 3.0, 9.0, 27.0)))
        printf("1 3 9 27: %g\n", value);
    if (report(geom_mean(&value, 3, 2.0, 8.0, 4.0)))
        printf("2 8 4: %g\n", value);
    if (report(geom_mean(&value, 3, 1e200, 1e200, 1e-300)))
        printf("1e200 1e200 1e-300: %g\n", value);
    if (report(geom_mean(&value, 2, 0.0, 5.0)))
        printf("0 5: %g\n", value);
    printf("\n");
}

// 6.5 - fast pow

void demo_pow(void)
{
    const double eps = 1e-9;
    double value = 0.0;

    printf("6.5 fast pow\n");
    if (report(fast_pow(&value, 2.0, 10, eps)))  printf("2^10 = %g\n", value);
    if (report(fast_pow(&value, 2.0, -3, eps)))  printf("2^-3 = %g\n", value);
    if (report(fast_pow(&value, -3.0, 3, eps)))  printf("(-3)^3 = %g\n", value);
    if (report(fast_pow(&value, 0.5, -2, eps)))  printf("0.5^-2 = %g\n", value);
    if (report(fast_pow(&value, 7.0, 0, eps)))   printf("7^0 = %g\n", value);
    printf("\n");
}

// 6.6 - dichotomy

void demo_dichotomy(void)
{
    double root = 0.0;

    printf("6.6 dichotomy\n");
    if (report(dichotomy(&root, 0.0, 2.0, 1e-3, f_sqrt2)))
        printf("x^2 - 2, [0, 2], eps 1e-3: %.6f\n", root);
    if (report(dichotomy(&root, 0.0, 2.0, 1e-9, f_sqrt2)))
        printf("x^2 - 2, [0, 2], eps 1e-9: %.10f\n", root);
    if (report(dichotomy(&root, -2.0, 0.0, 1e-6, f_sqrt2)))
        printf("x^2 - 2, [-2, 0], eps 1e-6: %.7f\n", root);
    if (report(dichotomy(&root, 0.0, 1.0, 1e-9, f_cos)))
        printf("cos x - x, [0, 1], eps 1e-9: %.10f\n", root);
    if (report(dichotomy(&root, 1.0, 2.0, 1e-5, f_cube)))
        printf("x^3 - x - 1, [1, 2], eps 1e-5: %.6f\n", root);
    printf("\n");
}

// errors

void demo_errors(void)
{
    int convex = 0, found_count = 0;
    double value = 0.0;
    const char *found[2];

    printf("errors\n");
    printf("convex null: ");         print_status(is_convex(NULL, 1e-9, 3, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0));
    printf("convex 2 points: ");     print_status(is_convex(&convex, 1e-9, 2, 0.0, 0.0, 1.0, 1.0));
    printf("convex eps 0: ");        print_status(is_convex(&convex, 0.0, 3, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0));
    printf("convex inf: ");          print_status(is_convex(&convex, 1e-9, 3, 0.0, 0.0, INFINITY, 0.0, 0.0, 1.0));
    printf("poly degree -1: ");      print_status(poly_value(&value, 1.0, -1));
    printf("poly overflow: ");       print_status(poly_value(&value, 1e300, 2, 1.0, 0.0, 0.0));
    printf("kaprekar base 1: ");     print_status(find_kaprekar(found, &found_count, 2, 1, 1, "1"));
    printf("kaprekar \"1G\": ");     print_status(find_kaprekar(found, &found_count, 2, 10, 1, "1G"));
    printf("kaprekar \"\": ");       print_status(find_kaprekar(found, &found_count, 2, 10, 1, ""));
    printf("kaprekar big: ");        print_status(find_kaprekar(found, &found_count, 2, 10, 1, "99999999999"));
    printf("kaprekar small array: ");print_status(find_kaprekar(found, &found_count, 2, 10, 3, "1", "2", "3"));
    printf("geom count 0: ");        print_status(geom_mean(&value, 0));
    printf("geom negative: ");       print_status(geom_mean(&value, 2, 4.0, -1.0));
    printf("pow 0^-2: ");            print_status(fast_pow(&value, 0.0, -2, 1e-9));
    printf("pow 10^400: ");          print_status(fast_pow(&value, 10.0, 400, 1e-9));
    printf("dichotomy no root: ");   print_status(dichotomy(&value, 3.0, 4.0, 1e-9, f_sqrt2));
    printf("dichotomy a > b: ");     print_status(dichotomy(&value, 2.0, 0.0, 1e-9, f_sqrt2));
    printf("dichotomy null f: ");    print_status(dichotomy(&value, 0.0, 2.0, 1e-9, NULL));
    printf("dichotomy eps 1e-30: "); print_status(dichotomy(&value, 0.0, 2.0, 1e-30, f_sqrt2));
}

int main(int argc, char *argv[])
{
    (void)argv;
    if (argc != 1) {
        printf("no args needed\n");
        return 1;
    }

    demo_convex();
    demo_poly();
    demo_kaprekar();
    demo_geom();
    demo_pow();
    demo_dichotomy();
    demo_errors();
    return 0;
}
