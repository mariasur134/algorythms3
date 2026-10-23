#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

typedef enum {
    STATUS_OK,             // ok
    STATUS_ERR_ARGC,       // wrong number of arguments
    STATUS_ERR_FLAG,       // unknown flag
    STATUS_ERR_NULL,       // NULL pointer passed to function
    STATUS_ERR_NAN,        // seed is not a number
    STATUS_ERR_OVERFLOW,   // seed > UINT_MAX or total length overflow
    STATUS_ERR_NOT_ASCII,  // string has non-ASCII chars (cyrillic, hieroglyphs, ...)
    STATUS_ERR_ALLOC       // malloc failed
} status_t;


// helpers
int is_digit_s(char c){
    return c >= '0' && c <= '9';
}

int is_alpha_s(char c){
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

char to_upper_s(char c){
    if (c >= 'a' && c <= 'z')
        return (char)(c - 'a' + 'A');
    return c;
}

// only ASCII allowed
status_t check_ascii(const char* s){
    if (s == NULL) return STATUS_ERR_NULL;

    for (size_t i = 0; s[i] != '\0'; i++)
        if ((unsigned char)s[i] > 127) return STATUS_ERR_NOT_ASCII;
    return STATUS_OK;
}

// parse unsigned int
status_t parse_uint(const char* s, unsigned int* res){
    if (s == NULL || res == NULL) return STATUS_ERR_NULL;
    if (s[0] == '\0') return STATUS_ERR_NAN;

    unsigned int v = 0;
    for (size_t i = 0; s[i] != '\0'; i++){
        if (!is_digit_s(s[i])) return STATUS_ERR_NAN;
        unsigned int d = (unsigned int)(s[i] - '0');
        if (v > (UINT_MAX - d) / 10) return STATUS_ERR_OVERFLOW;
        v = v * 10 + d;
    }

    *res = v;
    return STATUS_OK;
}


// flag <-l>
status_t str_len(const char* s, size_t* len){
    if (s == NULL || len == NULL) return STATUS_ERR_NULL;
    if (check_ascii(s) != STATUS_OK) return STATUS_ERR_NOT_ASCII;

    size_t n = 0;
    while (s[n] != '\0') n++;

    *len = n;
    return STATUS_OK;
}


// flag <-r>
status_t str_reverse(const char* s, char** out){
    if (s == NULL || out == NULL) return STATUS_ERR_NULL;
    if (check_ascii(s) != STATUS_OK) return STATUS_ERR_NOT_ASCII;

    size_t n = 0;
    str_len(s, &n);

    char* r = (char*)malloc(sizeof(char) * (n + 1));
    if (r == NULL) return STATUS_ERR_ALLOC;

    for (size_t i = 0; i < n; i++) r[i] = s[n - 1 - i];
    r[n] = '\0';

    *out = r;
    return STATUS_OK;
}


// flag <-u>  [odd positions -> upper]
status_t str_odd_upper(const char* s, char** out){
    if (s == NULL || out == NULL) return STATUS_ERR_NULL;
    if (check_ascii(s) != STATUS_OK) return STATUS_ERR_NOT_ASCII;

    size_t n = 0;
    str_len(s, &n);

    char* r = (char*)malloc(sizeof(char) * (n + 1));
    if (r == NULL) return STATUS_ERR_ALLOC;

    for (size_t i = 0; i < n; i++)
        r[i] = (i % 2 == 1) ? to_upper_s(s[i]) : s[i];
    r[n] = '\0';

    *out = r;
    return STATUS_OK;
}


// flag <-n>
status_t str_sort_types(const char* s, char** out){
    if (s == NULL || out == NULL) return STATUS_ERR_NULL;
    if (check_ascii(s) != STATUS_OK) return STATUS_ERR_NOT_ASCII;

    size_t n = 0;
    str_len(s, &n);

    char* r = (char*)malloc(sizeof(char) * (n + 1));
    if (r == NULL) return STATUS_ERR_ALLOC;

    size_t k = 0;
    for (size_t i = 0; i < n; i++)
        if (is_digit_s(s[i])) r[k++] = s[i];
    for (size_t i = 0; i < n; i++)
        if (is_alpha_s(s[i])) r[k++] = s[i];
    for (size_t i = 0; i < n; i++)
        if (!is_digit_s(s[i]) && !is_alpha_s(s[i])) r[k++] = s[i];
    r[k] = '\0';

    *out = r;
    return STATUS_OK;
}


// flag <-c>
status_t str_concat_random(const char* const* strs, int cnt, unsigned int seed, char** out){
    if (strs == NULL || out == NULL || cnt < 1) return STATUS_ERR_NULL;

    const char** ord = (const char**)malloc(sizeof(const char*) * (size_t)cnt);
    if (ord == NULL) return STATUS_ERR_ALLOC;

    size_t total = 0;
    for (int i = 0; i < cnt; i++){
        if (strs[i] == NULL){
            free(ord);
            return STATUS_ERR_NULL;
        }
        if (check_ascii(strs[i]) != STATUS_OK){
            free(ord);
            return STATUS_ERR_NOT_ASCII;
        }
        ord[i] = strs[i];

        size_t n = 0;
        str_len(strs[i], &n);
        if (total > SIZE_MAX - 1 - n){
            free(ord);
            return STATUS_ERR_OVERFLOW;
        }
        total += n;
    }

    // shuffle
    srand(seed);
    for (int i = cnt - 1; i > 0; i--){
        int j = rand() % (i + 1);
        const char* t = ord[i];
        ord[i] = ord[j];
        ord[j] = t;
    }

    char* r = (char*)malloc(sizeof(char) * (total + 1));
    if (r == NULL){
        free(ord);
        return STATUS_ERR_ALLOC;
    }

    size_t k = 0;
    for (int i = 0; i < cnt; i++)
        for (size_t j = 0; ord[i][j] != '\0'; j++)
            r[k++] = ord[i][j];
    r[k] = '\0';

    free(ord);
    *out = r;
    return STATUS_OK;
}


// output
void print_error(status_t s){
    switch (s){
        case STATUS_ERR_ARGC:     printf("Error: wrong number of arguments\n"); break;
        case STATUS_ERR_FLAG:     printf("Error: unknown flag\n"); break;
        case STATUS_ERR_NULL:     printf("Error: invalid parameters\n"); break;
        case STATUS_ERR_NAN:      printf("Error: seed is not a valid unsigned int\n"); break;
        case STATUS_ERR_OVERFLOW: printf("Error: overflow\n"); break;
        case STATUS_ERR_NOT_ASCII: printf("Error: only latin letters, digits and ASCII symbols are allowed\n"); break;
        case STATUS_ERR_ALLOC:    printf("Error: failed to allocate memory\n"); break;
        default:                  printf("Error: unknown error\n"); break;
    }
}

void print_len(status_t s, const char* str, size_t len){
    if (s != STATUS_OK){
        print_error(s);
        return;
    }
    printf("Length of \"%s\": %zu\n", str, len);
}

void print_str(status_t s, const char* name, const char* res){
    if (s != STATUS_OK){
        print_error(s);
        return;
    }
    printf("%s: %s\n", name, res);
}

#endif
