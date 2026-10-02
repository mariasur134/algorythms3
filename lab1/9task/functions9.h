#ifndef FUNCTIONS9_H
#define FUNCTIONS9_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define FIXED_SIZE 20
#define RAND_SIZE_MIN 10
#define RAND_SIZE_MAX 10000
#define RAND_VALUE_MIN (-1000)
#define RAND_VALUE_MAX 1000

typedef enum {
    STATUS_OK,                // allgood
    STATUS_ERR_NULL_PTR,      // NULL
    STATUS_ERR_ARGC,          // argc != 3
    STATUS_ERR_NAN,           // str in nan
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_RANGE_ORDER,   // a > b
    STATUS_ERR_ALLOC          // couldn't allocate memory
} status_t;

// input
// int
status_t parse_integer(const char *str, long long *result)
{
    if (str == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;

    const size_t len = strlen(str);
    if (len == 0)
        return STATUS_ERR_NAN;

    size_t i = (str[0] == '-' || str[0] == '+') ? 1 : 0;
    if (i == len)
        return STATUS_ERR_NAN;              // only a sign ws given = bad
    for (; i < len; i++)
        if (!isdigit((unsigned char)str[i])) // imposter not a digit here
            return STATUS_ERR_NAN;

    char *end = NULL;
    const long long value = strtoll(str, &end, 10);
    if (end == str || *end != '\0')
        return STATUS_ERR_NAN;
    if (value == LLONG_MAX || value == LLONG_MIN)
        return STATUS_ERR_OVERFLOW;

    *result = value;
    return STATUS_OK;
}

status_t check_int_range(const long long value)
{
    if (value < INT_MIN || value > INT_MAX)
        return STATUS_ERR_OVERFLOW;
    return STATUS_OK;
}

// filling arrays

// filling array[0..size-1] with random numbers from a to b
status_t fill_array_rand(const int a, const int b, int *array, const int size)
{
    if (array == NULL)
        return STATUS_ERR_NULL_PTR;

    const long long width = (long long)b - (long long)a + 1;   // long long to avoid overflow of int
    for (int i = 0; i < size; i++)
        array[i] = (int)(a + rand() % width);

    return STATUS_OK;
}

// random number in [min_size, max_size]
int random_size(const int min_size, const int max_size)
{
    return min_size + rand() % (max_size - min_size + 1);
}

// finding min and max (single pass through)
status_t find_min_max_indices(const int *array, const int size, int *min_idx, int *max_idx)
{
    if (array == NULL || min_idx == NULL || max_idx == NULL)
        return STATUS_ERR_NULL_PTR;

    int mi = 0, ma = 0;
    for (int i = 1; i < size; i++) {
        if (array[i] < array[mi]) mi = i;
        if (array[i] > array[ma]) ma = i;
    }
    *min_idx = mi;
    *max_idx = ma;
    return STATUS_OK;
}

status_t swap_min_max(int *array, const int size)
{
    int min_idx = 0, max_idx = 0;
    const status_t st = find_min_max_indices(array, size, &min_idx, &max_idx);
    if (st != STATUS_OK)
        return st;

    const int tmp = array[min_idx];
    array[min_idx] = array[max_idx];
    array[max_idx] = tmp;
    return STATUS_OK;
}

// 9.2 closest element through qsort + bin searh

int compare_int(const void *x, const void *y)
{
    const int a = *(const int *)x;
    const int b = *(const int *)y;
    return (a > b) - (a < b);
}

// finding closest (nearest) element
int nearest_in_sorted(const int *sorted, const int size, const int target)
{
    int lo = 0, hi = size;
    while (lo < hi) {
        const int mid = lo + (hi - lo) / 2;
        if (sorted[mid] < target)
            lo = mid + 1;
        else
            hi = mid;
    }

    if (lo == 0) return sorted[0];
    if (lo == size) return sorted[size - 1];

    const int lower = sorted[lo - 1];
    const int upper = sorted[lo];
    return (target - lower <= upper - target) ? lower : upper;
}

// c[i] = a[i] + nearest to a[i] from sorted_b
status_t generate_c(const int *a, const int size_a, const int *sorted_b, const int size_b, int *c)
{
    if (a == NULL || sorted_b == NULL || c == NULL)
        return STATUS_ERR_NULL_PTR;

    for (int i = 0; i < size_a; i++)
        c[i] = a[i] + nearest_in_sorted(sorted_b, size_b, a[i]);

    return STATUS_OK;
}

#endif