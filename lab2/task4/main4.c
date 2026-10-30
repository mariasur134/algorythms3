#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "functions4.h"


char* read_token(void){
    int ch;
    while ((ch = getchar()) != EOF && isspace(ch));
    if (ch == EOF) return NULL;

    size_t cap = 32;
    size_t len = 0;
    char* buf = (char*)malloc(cap);
    if (buf == NULL) return NULL;

    buf[len++] = (char)ch;
    while ((ch = getchar()) != EOF && !isspace(ch)){
        if (len + 1 >= cap){
            size_t new_cap = cap * 2;
            char* tmp = (char*)realloc(buf, new_cap);
            if (tmp == NULL){ free(buf); return NULL; }
            buf = tmp;
            cap = new_cap;
        }
        buf[len++] = (char)ch;
    }
    buf[len] = '\0';
    return buf;
}


int parse_ull(const char* s, unsigned long long* out){
    if (s == NULL || *s == '\0') return 0;
    if (s[0] == '-') return 0;
    if (isspace((unsigned char)s[0])) return 0;

    char* end = NULL;
    unsigned long long val = strtoull(s, &end, 10);
    if (end == s || *end != '\0') return 0;

    *out = val;
    return 1;
}


void free_files(file_desc_t* files, int n){
    if (files == NULL) return;
    for (int i = 0; i < n; i++)
        free(files[i].name);
    free(files);
}


int main(void){
    // read n and capacity
    char* tok_n   = read_token();
    char* tok_cap = read_token();

    if (tok_n == NULL || tok_cap == NULL){
        printf("Error: failed to read n and capacity\n");
        free(tok_n);
        free(tok_cap);
        return 1;
    }

    unsigned long long n_ull = 0, cap = 0;
    if (!parse_ull(tok_n, &n_ull) || n_ull < 1){
        printf("Error: invalid n\n");
        free(tok_n); free(tok_cap);
        return 1;
    }
    if (!parse_ull(tok_cap, &cap)){
        printf("Error: invalid capacity\n");
        free(tok_n); free(tok_cap);
        return 1;
    }
    free(tok_n);
    free(tok_cap);

    int n = (int)n_ull;
    if ((unsigned long long)n != n_ull){
        printf("Error: n too large\n");
        return 1;
    }

    file_desc_t* files = (file_desc_t*)calloc((size_t)n, sizeof(file_desc_t));
    if (files == NULL){
        printf("Error: failed to allocate memory\n");
        return 1;
    }

    // read each file
    for (int i = 0; i < n; i++){
        char* name    = read_token();
        char* tok_sz  = read_token();
        char* tok_imp = read_token();

        if (name == NULL || tok_sz == NULL || tok_imp == NULL){
            printf("Error: unexpected end of input at file %d\n", i + 1);
            free(name); free(tok_sz); free(tok_imp);
            free_files(files, i);
            return 1;
        }

        unsigned long long sz = 0, imp = 0;
        if (!parse_ull(tok_sz, &sz) || sz < 1){
            printf("Error: invalid size for file '%s'\n", name);
            free(name); free(tok_sz); free(tok_imp);
            free_files(files, i);
            return 1;
        }
        if (!parse_ull(tok_imp, &imp)){
            printf("Error: invalid importance for file '%s'\n", name);
            free(name); free(tok_sz); free(tok_imp);
            free_files(files, i);
            return 1;
        }

        free(tok_sz);
        free(tok_imp);

        files[i].name       = name;
        files[i].size       = sz;
        files[i].importance = imp;
    }

    // run knapsack
    backup_result_t result;
    status_t s = select_files(files, n, cap, &result);
    print_result(s, &result);

    free_result(&result);
    free_files(files, n);

    return (s == STATUS_OK) ? 0 : 1;
}
