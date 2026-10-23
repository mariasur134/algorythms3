#define _XOPEN_SOURCE 700
#include "functions1.h"

void print_hint(void){
    printf("Usage:\n");
    printf("  <-l|-r|-u|-n> <string>\n");
    printf("  -c <string> <seed> [string ...]\n\n");
}

int is_valid_flag(const char* f, char* out){
    if (f == NULL || out == NULL) return 0;
    if (f[0] != '-' || f[1] == '\0' || f[2] != '\0') return 0;

    char c = f[1];
    if (c == 'l' || c == 'r' || c == 'u' || c == 'n' || c == 'c'){
        *out = c;
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[]){
    print_hint();

    if (argc < 3){
        print_error(STATUS_ERR_ARGC);
        return 1;
    }

    char flag = 0;
    if (!is_valid_flag(argv[1], &flag)){
        print_error(STATUS_ERR_FLAG);
        return 1;
    }

    if ((flag != 'c' && argc != 3) || (flag == 'c' && argc < 4)){
        print_error(STATUS_ERR_ARGC);
        return 1;
    }

    status_t s = STATUS_OK;

    switch (flag){
        case 'l': {
            size_t len = 0;
            s = str_len(argv[2], &len);
            print_len(s, argv[2], len);
            break;
        }
        case 'r': {
            char* r = NULL;
            s = str_reverse(argv[2], &r);
            print_str(s, "Reversed", r);
            free(r);
            break;
        }
        case 'u': {
            char* r = NULL;
            s = str_odd_upper(argv[2], &r);
            print_str(s, "Odd upper", r);
            free(r);
            break;
        }
        case 'n': {
            char* r = NULL;
            s = str_sort_types(argv[2], &r);
            print_str(s, "Digits, letters, others", r);
            free(r);
            break;
        }
        case 'c': {
            unsigned int seed = 0;
            s = parse_uint(argv[3], &seed);
            if (s != STATUS_OK){
                print_error(s);
                break;
            }

            // strings: argv[2], argv[4], argv[5], ...
            int cnt = argc - 3;
            const char** strs = (const char**)malloc(sizeof(const char*) * (size_t)cnt);
            if (strs == NULL){
                s = STATUS_ERR_ALLOC;
                print_error(s);
                break;
            }
            strs[0] = argv[2];
            for (int i = 4; i < argc; i++) strs[i - 3] = argv[i];

            char* r = NULL;
            s = str_concat_random(strs, cnt, seed, &r);
            printf("Seed: %u\n", seed);
            print_str(s, "Concat", r);

            free(r);
            free(strs);
            break;
        }
    }

    return (s == STATUS_OK) ? 0 : 1;
}
