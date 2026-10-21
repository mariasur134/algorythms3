#include <stdio.h>
#include "functions8.h"


int main(void){
    char* r = NULL;
    status_t s;

    // 1. carry + leading zeros
    printf("1) base 10: 999 + 1 + 0000\n");
    s = sum(&r, 10, 3, "999", "1", "0000");
    print_sum(s, 10, r);
    free(r);
    r = NULL;

    // 2. hex, mixed case
    printf("\n2) base 16: ff + Ff + 2\n");
    s = sum(&r, 16, 3, "ff", "Ff", "2");
    print_sum(s, 16, r);
    free(r);
    r = NULL;

    // 3. very long numbers (do not fit in long long)
    printf("\n3) base 10: 99999999999999999999999999999999 + 1 + 00000000000000000000000000000001\n");
    s = sum(&r, 10, 3, "99999999999999999999999999999999", "1",
            "00000000000000000000000000000001");
    print_sum(s, 10, r);
    free(r);
    r = NULL;

    // 4. error: digit >= base
    printf("\n4) base 10: 123 + 12A\n");
    s = sum(&r, 10, 2, "123", "12A");
    print_sum(s, 10, r);
    free(r);
    r = NULL;

    // 5. error: invalid base
    printf("\n5) base 37: 1 + 1\n");
    s = sum(&r, 37, 2, "1", "1");
    print_sum(s, 37, r);
    free(r);
    r = NULL;

    return 0;
}
