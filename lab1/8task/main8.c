#include <stdio.h>
#include <string.h>
#include "functions8.h"

void print_hint(void)
{
    printf("usage: <input_file> <output_file>\n");
    printf("input: numbers in bases 2..36 separated by spaces, tabs or newlines\n");
    printf("output: number without leading zeros, its minimal base, decimal value\n");
    printf("invalid numbers (bad symbol, overflow) are skipped\n\n");
}

void print_status(const status_t status)
{
    switch (status) {
        case STATUS_OK:        printf("all good\n"); break;
        case STATUS_ERR_ALLOC: printf("error: failed to allocate memory\n"); break;
        case STATUS_ERR_NULL_PTR: printf("error: null pointer\n"); break;
        default:               printf("error: unknown status\n"); break;
    }
}

int main(int argc, char *argv[])
{
    print_hint();

    if (argc != 3) {
        printf("error: invalid number of arguments (2 expected)\n");
        return 1;
    }

    if (strcmp(argv[1], argv[2]) == 0) {
        printf("error: input and output files must be different\n");
        return 1;
    }

    FILE *in = fopen(argv[1], "r");
    if (in == NULL) {
        printf("error: can't open input file\n");
        return 1;
    }

    FILE *out = fopen(argv[2], "w");
    if (out == NULL) {
        printf("error: can't open output file\n");
        fclose(in);
        return 1;
    }

    const status_t st = process_file(in, out);
    print_status(st);

    fclose(in);
    fclose(out);
    return (st == STATUS_OK) ? 0 : 1;
}