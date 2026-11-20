#include "functions1.h"

// compute + print case, free result
void run_case(const int n, const int r){
    char* res = NULL;
    status_t s = decimal_to_base_2r(n, r, &res);
    print_base_2r(s, n, r, res);
    if (s == STATUS_OK)
        free(res);
}

int main(void){
    // 1. the same number in every base 2^r
    printf("1) 110 in r = 1..5\n");
    for (int r = MIN_R; r <= MAX_R; r++)
        run_case(110, r);

    // 2. zero
    printf("\n2) zero\n");
    run_case(0, 1);
    run_case(-0, 1);

    // 3. negative numbers
    printf("\n3) negative numbers\n");
    run_case(-255, 4);
    run_case(-1, 1);
    run_case(-1024, 3);

    // 4. INT_MAX and INT_MIN
    printf("\n4) INT_MAX, INT_MIN\n");
    run_case(INT_MAX, 5);
    run_case(INT_MAX, 1);
    run_case(INT_MIN, 5);
    run_case(INT_MIN, 1);

    // 5. error: wrong r
    printf("\n5) wrong r\n");
    run_case(110, 0);
    run_case(110, 6);
    run_case(110, -3);

    // 6. error: null result pointer
    printf("\n6) null result pointer\n");
    print_base_2r(decimal_to_base_2r(5, 2, NULL), 5, 2, NULL);

    return 0;
}
