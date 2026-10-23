#define _XOPEN_SOURCE 700   // for realpath
#include "functions2.h"

void print_hint(void){
    printf("Usage:\n");
    printf("  <input file> <-a|/a|-d|/d> <output file>\n\n");
}

void print_status(status_t s, size_t bad_line){
    switch (s){
        case STATUS_OK:            printf("done\n"); break;
        case STATUS_ERR_NULL:      printf("Error: NULL pointer\n"); break;
        case STATUS_ERR_FLAG:      printf("Error: wrong flag\n"); break;
        case STATUS_ERR_SAME_FILE: printf("Error: input and output files must be different\n"); break;
        case STATUS_ERR_OPEN_IN:   printf("Error: cannot open input file\n"); break;
        case STATUS_ERR_OPEN_OUT:  printf("Error: cannot open output file\n"); break;
        case STATUS_ERR_READ:      printf("Error: cannot read input file\n"); break;
        case STATUS_ERR_WRITE:     printf("Error: cannot write output file\n"); break;
        case STATUS_ERR_FORMAT:
            printf("Error: wrong data in line %zu (expected: id name surname salary)\n", bad_line);
            break;
        case STATUS_ERR_ALLOC:     printf("Error: failed to allocate memory\n"); break;
        default:                   printf("Error: unknown error\n"); break;
    }
}

// -a, /a, -d, /d
int parse_flag(const char* s, char* flag){
    if (s == NULL || flag == NULL) return 0;
    if ((s[0] != '-' && s[0] != '/') || s[1] == '\0' || s[2] != '\0') return 0;
    if (s[1] != 'a' && s[1] != 'd') return 0;
    *flag = s[1];
    return 1;
}

status_t check_different(const char* in, const char* out){
    char real_in[PATH_MAX];
    char real_out[PATH_MAX];

    // output file doesn't exist yet - nothing to compare
    if (realpath(out, real_out) == NULL) return STATUS_OK;
    if (realpath(in, real_in) != NULL && strcmp(real_in, real_out) == 0)
        return STATUS_ERR_SAME_FILE;
    return STATUS_OK;
}

status_t run(const char* path_in, char flag, const char* path_out, size_t* bad_line){
    status_t s = check_different(path_in, path_out);
    if (s != STATUS_OK) return s;

    FILE* in = fopen(path_in, "r");
    if (in == NULL) return STATUS_ERR_OPEN_IN;

    // read everything first: if input is wrong, output file is not created
    Employee* arr = NULL;
    size_t cnt = 0;
    s = read_employees(in, &arr, &cnt, bad_line);
    fclose(in);
    if (s != STATUS_OK) return s;

    s = sort_employees(arr, cnt, flag);
    if (s != STATUS_OK){
        free(arr);
        return s;
    }

    FILE* out = fopen(path_out, "w");
    if (out == NULL){
        free(arr);
        return STATUS_ERR_OPEN_OUT;
    }

    s = write_employees(out, arr, cnt);
    if (fclose(out) != 0 && s == STATUS_OK) s = STATUS_ERR_WRITE;

    free(arr);
    return s;
}

int main(int argc, char* argv[]){
    print_hint();

    if (argc != 4){
        printf("Error: wrong number of arguments\n");
        return 1;
    }

    char flag = 0;
    if (!parse_flag(argv[2], &flag)){
        print_status(STATUS_ERR_FLAG, 0);
        return 1;
    }

    size_t bad_line = 0;
    status_t s = run(argv[1], flag, argv[3], &bad_line);
    print_status(s, bad_line);
    return (s == STATUS_OK) ? 0 : 1;
}
