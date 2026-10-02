#include "functions1.h"

void print_hint(void){
    printf("Usage: <flag> <number>\n");
    printf("Flags: -h -p -s -e -a -f (or '/')\n");
    printf("Number: +-integer\n\n");
}

int is_valid_flag(const char* flag_str, char* out_flag){
    if (flag_str == NULL || out_flag == NULL)
        return 0;

    if (strlen(flag_str) != 2 || (flag_str[0] != '-' && flag_str[0] != '/'))
        return 0;

    char c = flag_str[1];
    if (c == 'h' || c == 'p' || c == 's' || c == 'e' || c == 'a' || c == 'f'){
        *out_flag = c;
        return 1;
    }
    return 0;
}

void print_parse_error(status_t status){
    switch (status){
        case STATUS_ERR_NULL_PTR: printf("Error: null pointer\n"); break;
        case STATUS_ERR_NAN:      printf("Error: invalid number\n"); break;
        case STATUS_ERR_OVERFLOW: printf("Error: overflow\n"); break;
        default:                  printf("Error: unknown parsing error\n"); break;
    }
}

int main(int argc, char *argv[]){
    print_hint();

    if (argc != 3){
        printf("Error: expected 2 arguments\n");
        return 1;
    }

    char flag = 0;
    if (!is_valid_flag(argv[1], &flag)){
        printf("Error: unknown flag: %s\n", argv[1]);
        return 1;
    }

    long long x = 0;
    status_t parse_status = parse_number(argv[2], &x);
    if (parse_status != STATUS_OK){
        print_parse_error(parse_status);
        return 1;
    }

    switch (flag){
        case 'h': {
            long long arr[MAX_X_H];
            int size = 0;
            status_t s = multiples_in_range(x, arr, &size);
            print_multiples_in_range(s, x, arr, size);
            break;
        }
        case 'p': {
            int prime = 0;
            status_t s = check_prime(x, &prime);
            print_prime(s, x, prime);
            break;
        }
        case 's': {
            int digits[MAX_HEX_DIG];
            int count = 0;
            status_t s = split_hex(x, digits, &count);
            print_hex(s, x, digits, count);
            break;
        }
        case 'e': {
            long long table[MAX_X_E][MAX_X_E];
            int rows = 0, cols = 0;
            status_t s = powers_table(x, table, &rows, &cols);
            print_powers_table(s, x, table, rows, cols);
            break;
        }
        case 'a': {
            long long result = 0;
            status_t s = sum_natural(x, &result);
            print_sum(s, x, result);
            break;
        }
        case 'f': {
            long long result = 0;
            status_t s = factorial_iter(x, &result);
            print_factorial(s, x, result);
            break;
        }
    }
    return 0;
}