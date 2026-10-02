#include <stdio.h>
#include "functions4.h"

void print_hint(void)
{
    printf("usage: <flag> <input_file> <output_file (optional)>\n");
    printf("flags: -d -i -s -a (or /); with 'n' (-nd -ni -ns -na) for naming the output file\n");
    printf("  -d  remove digits\n");
    printf("  -i  count latin letters per line\n");
    printf("  -s  count symbols that are not letters, digits or spaces, per line\n");
    printf("  -a  replace characters other than digits/space/'\\n' wiht ASCII hex code\n");
    printf("without 'n' output file will be out_<input_file>\n\n");
}

void print_status(status_t status)
{
    switch (status) {
        case STATUS_OK:                printf("all good\n"); break;
        case STATUS_ERR_ARGC:           printf("error: invalid number of arguments\n"); break;
        case STATUS_ERR_FLAG_UNKNOWN:   printf("error: invalid flag\n"); break;
        case STATUS_ERR_OPEN_INPUT:     printf("error: can't open input file\n"); break;
        case STATUS_ERR_OPEN_OUTPUT:    printf("error: can't open output file\n"); break;
        case STATUS_ERR_ALLOC:          printf("error: failed to allocate memory\n"); break;
        case STATUS_ERR_NULL_PTR:       printf("error: null pointer\n"); break;
        default:                        printf("error: unknown status\n"); break;
    }
}

int main(int argc, char *argv[])
{
    print_hint();

    if (argc < 3 || argc > 4) {
        print_status(STATUS_ERR_ARGC);
        return 1;
    }

    char flag = 0;
    int has_n = 0;
    status_t st = parse_flag(argv[1], &flag, &has_n);
    if (st != STATUS_OK) {
        print_status(st);
        return 1;
    }

    const int need_argc = has_n ? 4 : 3;
    if (argc != need_argc) {
        print_status(STATUS_ERR_ARGC);
        return 1;
    }

    char *output_path = NULL;
    int output_allocated = 0;

    if (has_n) {
        output_path = argv[3];
    } else {
        st = build_output_path(argv[2], &output_path);
        if (st != STATUS_OK) {
            print_status(st);
            return 1;
        }
        output_allocated = 1;
    }

    switch (flag) {
        case 'd': st = flag_d(argv[2], output_path); break;
        case 'i': st = flag_i(argv[2], output_path); break;
        case 's': st = flag_s(argv[2], output_path); break;
        case 'a': st = flag_a(argv[2], output_path); break;
        default:  st = STATUS_ERR_FLAG_UNKNOWN; break;
    }

    print_status(st);

    if (output_allocated)
        free(output_path);

    return (st == STATUS_OK) ? 0 : 1;
}