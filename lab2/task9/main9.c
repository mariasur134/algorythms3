#define EPS 1e-13   // change here (must be < 5e-13, see functions9.h)
#include "functions9.h"

int main(void){
    int* r = NULL;
    status_t s;

    // 1. base 10
    {
        double n[] = {0.125, 0.1, 0.3};
        printf("1)\n");
        s = check_fin(&r, 10, 3, 0.125, 0.1, 0.3);
        print_res(s, 10, n, r, 3);
        free(r);
        r = NULL;
    }

    // 2. base 2: 0.1 is infinite in binary
    {
        double n[] = {0.5, 0.125, 0.1};
        printf("\n2)\n");
        s = check_fin(&r, 2, 3, 0.5, 0.125, 0.1);
        print_res(s, 2, n, r, 3);
        free(r);
        r = NULL;
    }

    // 3. base 3: 1/3 = 0.1 in base 3
    {
        double n[] = {1.0 / 3, 2.0 / 9, 0.5};
        printf("\n3)\n");
        s = check_fin(&r, 3, 3, 1.0 / 3, 2.0 / 9, 0.5);
        print_res(s, 3, n, r, 3);
        free(r);
        r = NULL;
    }

    // 4. error: number not in (0; 1)
    printf("\n4) number 1.0\n");
    s = check_fin(&r, 10, 2, 0.5, 1.0);
    print_res(s, 10, NULL, r, 2);
    free(r);
    r = NULL;

    // 5. error: base < 2
    printf("\n5) base 1\n");
    s = check_fin(&r, 1, 1, 0.5);
    print_res(s, 1, NULL, r, 1);
    free(r);
    r = NULL;

    return 0;
}
