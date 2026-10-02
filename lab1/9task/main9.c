#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "functions9.h"

void print_hint(void)
{
    printf("usage: <a> <b>  (a <= b, int)\n");
    printf("9.1: fills a %d-element array with random numbers from [a, b], swaps min and max\n", FIXED_SIZE);
    printf("9.2: builds arrays A, B of random size [%d, %d] from [%d, %d], C[i] = A[i] + nearest value in B\n\n",
           RAND_SIZE_MIN, RAND_SIZE_MAX, RAND_VALUE_MIN, RAND_VALUE_MAX);
}

void print_status(const status_t status)
{
    switch (status) {
        case STATUS_ERR_ARGC:        printf("error: invalid num of arg (2 expected)\n"); break;
        case STATUS_ERR_NAN:         printf("error: invalid integer\n"); break;
        case STATUS_ERR_OVERFLOW:    printf("error: number out of range\n"); break;
        case STATUS_ERR_RANGE_ORDER: printf("error: a must be <= b\n"); break;
        case STATUS_ERR_ALLOC:       printf("error: failed to allocate memory\n"); break;
        case STATUS_ERR_NULL_PTR:    printf("error: null pointer\n"); break;
        default:                     printf("error: unknown status\n"); break;
    }
}

void print_array(const char *name, const int *array, const int size)
{
    printf("%s:", name);
    for (int i = 0; i < size; i++)
        printf(" %d", array[i]);
    printf("\n");
}

int main(int argc, char *argv[])
{
    print_hint();

    if (argc != 3) {
        print_status(STATUS_ERR_ARGC);
        return 1;
    }

    long long a_ll = 0, b_ll = 0;
    status_t st = parse_integer(argv[1], &a_ll);
    if (st != STATUS_OK) { print_status(st); return 1; }

    st = parse_integer(argv[2], &b_ll);
    if (st != STATUS_OK) { print_status(st); return 1; }

    if ((st = check_int_range(a_ll)) != STATUS_OK) { print_status(st); return 1; }
    if ((st = check_int_range(b_ll)) != STATUS_OK) { print_status(st); return 1; }

    if (a_ll > b_ll) {
        print_status(STATUS_ERR_RANGE_ORDER);
        return 1;
    }

    const int a = (int)a_ll, b = (int)b_ll;
    srand((unsigned int)time(NULL));

    // 9.1
    int fixed_array[FIXED_SIZE];
    fill_array_rand(a, b, fixed_array, FIXED_SIZE);

    printf("9.1:\n");
    print_array("before", fixed_array, FIXED_SIZE);

    st = swap_min_max(fixed_array, FIXED_SIZE);
    if (st != STATUS_OK) { print_status(st); return 1; }

    print_array("after ", fixed_array, FIXED_SIZE);

    // 9.2
    const int size_a = random_size(RAND_SIZE_MIN, RAND_SIZE_MAX);
    const int size_b = random_size(RAND_SIZE_MIN, RAND_SIZE_MAX);

    int *array_a = (int *)malloc(sizeof(int) * size_a);
    int *array_b = (int *)malloc(sizeof(int) * size_b);
    int *array_c = (int *)malloc(sizeof(int) * size_a);

    if (array_a == NULL || array_b == NULL || array_c == NULL) {
        print_status(STATUS_ERR_ALLOC);
        free(array_a);
        free(array_b);
        free(array_c);
        return 1;
    }

    fill_array_rand(RAND_VALUE_MIN, RAND_VALUE_MAX, array_a, size_a);
    fill_array_rand(RAND_VALUE_MIN, RAND_VALUE_MAX, array_b, size_b);

    qsort(array_b, size_b, sizeof(int), compare_int);

    st = generate_c(array_a, size_a, array_b, size_b, array_c);
    if (st != STATUS_OK) {
        print_status(st);
        free(array_a);
        free(array_b);
        free(array_c);
        return 1;
    }

    const int shown = size_a < 10 ? size_a : 10;
    printf("\n9.2:\nsize A = %d, size B = %d (showing first %d elements)\n", size_a, size_b, shown);
    print_array("A", array_a, shown);
    print_array("C", array_c, shown);

    free(array_a);
    free(array_b);
    free(array_c);

    return 0;
}