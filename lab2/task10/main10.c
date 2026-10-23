#include "functions10.h"

#define EPS 1e-9

int main(void){
    double* g = NULL;
    status_t s;

    // 1. 2x^2 + 3x + 1, a = 4
    {
        double f[] = {2.0, 3.0, 1.0};
        printf("1) f(x) = 2x^2 + 3x + 1, a = 4\n");
        s = decompose(EPS, 4.0, &g, 2, 2.0, 3.0, 1.0);
        print_result(s, f, g, 2, 4.0, 6.0, EPS);
        free(g);
        g = NULL;
    }

    // 2. 4x^3 + 3x^2 + 2x + 1, a = -2
    {
        double f[] = {4.0, 3.0, 2.0, 1.0};
        printf("\n2) f(x) = 4x^3 + 3x^2 + 2x + 1, a = -2\n");
        s = decompose(EPS, -2.0, &g, 3, 4.0, 3.0, 2.0, 1.0);
        print_result(s, f, g, 3, -2.0, 1.5, EPS);
        free(g);
        g = NULL;
    }

    // 3. special: degree 0
    {
        double f[] = {5.0};
        printf("\n3) f(x) = 5, a = 10 (degree 0)\n");
        s = decompose(EPS, 10.0, &g, 0, 5.0);
        print_result(s, f, g, 0, 10.0, 3.0, EPS);
        free(g);
        g = NULL;
    }

    // 4. error: negative degree
    printf("\n4) degree = -1\n");
    s = decompose(EPS, 1.0, &g, -1);
    print_result(s, NULL, g, -1, 1.0, 0.0, EPS);
    free(g);
    g = NULL;

    // 5. error: eps <= 0
    printf("\n5) eps = 0\n");
    s = decompose(0.0, 1.0, &g, 1, 1.0, 1.0);
    print_result(s, NULL, g, 1, 1.0, 0.0, EPS);
    free(g);
    g = NULL;

    return 0;
}
