#include <stdio.h>
#include "functions2.h"

typedef struct {
    const char *name;
    const char *method;
    calc_func_t func;
} task_t;

void print_result(const task_t *task, const status_t status, const long double value)
{
    printf("%-8s %-9s ", task->name, task->method);
    switch (status) {
        case STATUS_OK:
            printf("%.12Lf\n", value);
            break;
        case STATUS_ERR_CONVERGENCE:
            printf("error: no convergence\n");
            break;
        case STATUS_ERR_MEMORY:
            printf("error: memory allocation failed\n");
            break;
        case STATUS_ERR_NULL_PTR:
            printf("error: null pointer\n");
            break;
        case STATUS_ERR_RANGE:
            printf("error: epsilon out of range\n");
            break;
        default:
            printf("error: unknown status\n");
            break;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <epsilon>, epsilon in [1e-4, 1e-1]\n", argv[0]);
        return 1;
    }

    long double epsilon = 0.0L;
    switch (parse_epsilon(argv[1], &epsilon)) {
        case STATUS_OK:
            break;
        case STATUS_ERR_RANGE:
            printf("Error: epsilon must be in [1e-4, 1e-1]\n");
            return 1;
        default:
            printf("Error: invalid epsilon\n");
            return 1;
    }

    const task_t tasks[] = {
        {"E",       "limit", e_limit},     {"E",       "series", e_series},     {"E",       "equation", e_equation},
        {"PI",      "limit", pi_limit},    {"PI",      "series", pi_series},    {"PI",      "equation", pi_equation},
        {"LN(2)",   "limit", ln2_limit},   {"LN(2)",   "series", ln2_series},   {"LN(2)",   "equation", ln2_equation},
        {"SQRT(2)", "limit", sqrt2_limit}, {"SQRT(2)", "series", sqrt2_series}, {"SQRT(2)", "equation", sqrt2_equation},
        {"GAMMA",   "limit", gamma_limit}, {"GAMMA",   "series", gamma_series}, {"GAMMA",   "equation", gamma_equation},
    };
    const int count = sizeof(tasks) / sizeof(tasks[0]);

    printf("epsilon = %.12Lf\n", epsilon);
    for (int i = 0; i < count; i++) {
        if (i % 3 == 0)
            printf("\n");
        long double value = 0.0L;
        const status_t status = tasks[i].func(epsilon, &value);
        print_result(&tasks[i], status, value);
    }
    return 0;
}
