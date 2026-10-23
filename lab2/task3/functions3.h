#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef enum {
    STATUS_OK,          // ok
    STATUS_ERR_NULL,    // NULL pointer
    STATUS_ERR_EMPTY,   // empty substring
    STATUS_ERR_CNT,     // count of files < 1
    STATUS_ERR_FILE,    // cannot open file
    STATUS_ERR_READ,    // error while reading file
    STATUS_ERR_NOT_ASCII, // non-ASCII chars  in substring or file
    STATUS_ERR_ALLOC    // malloc / realloc failed
} status_t;

// one occurence (which line which ind)
typedef struct {
    int line;   // from 1
    int pos;    // from 1
} Pos;

// result for 1 file
typedef struct {
    const char* path;
    Pos* arr;
    int cnt;
} FileRes;


size_t len_s(const char* s){
    size_t n = 0;
    while (s[n] != '\0') n++;
    return n;
}

// only ASCII chars
status_t check_ascii(const char* s, size_t n){
    if (s == NULL) return STATUS_ERR_NULL;

    for (size_t i = 0; i < n; i++)
        if ((unsigned char)s[i] > 127) return STATUS_ERR_NOT_ASCII;
    return STATUS_OK;
}

// read whole file into memory (result must be freed)
status_t read_file(const char* path, char** text, size_t* len){
    if (path == NULL || text == NULL || len == NULL) return STATUS_ERR_NULL;

    FILE* f = fopen(path, "r");
    if (f == NULL) return STATUS_ERR_FILE;

    size_t cap = 64, n = 0;
    char* buf = (char*)malloc(sizeof(char) * cap);
    if (buf == NULL){
        fclose(f);
        return STATUS_ERR_ALLOC;
    }

    int c;
    while ((c = fgetc(f)) != EOF){
        if (n + 1 >= cap){
            cap *= 2;
            char* t = (char*)realloc(buf, sizeof(char) * cap);
            if (t == NULL){
                free(buf);
                fclose(f);
                return STATUS_ERR_ALLOC;
            }
            buf = t;
        }
        buf[n++] = (char)c;
    }

    // EOF (or possible error?)
    if (ferror(f)){
        free(buf);
        fclose(f);
        return STATUS_ERR_READ;
    }
    buf[n] = '\0';
    fclose(f);

    *text = buf;
    *len = n;
    return STATUS_OK;
}

// does sub start at t[i]?
// [whole text, length, position, sub - pattern, m - pattern length]
int match_at(const char* t, size_t n, size_t i, const char* sub, size_t m){
    if (i + m > n) return 0;
    for (size_t j = 0; j < m; j++)
        if (t[i + j] != sub[j]) return 0;
    return 1;
}

// find all occurrences of sub in text (arr must be freed)
status_t search_text(const char* t, size_t n, const char* sub, Pos** arr, int* cnt){
    if (t == NULL || sub == NULL || arr == NULL || cnt == NULL) return STATUS_ERR_NULL;

    size_t m = len_s(sub);
    int cap = 4, k = 0;
    Pos* a = (Pos*)malloc(sizeof(Pos) * (size_t)cap);
    if (a == NULL) return STATUS_ERR_ALLOC;

    int line = 1, pos = 1;
    for (size_t i = 0; i < n; i++){
        if (match_at(t, n, i, sub, m)){
            if (k == cap){
                cap *= 2;
                Pos* tmp = (Pos*)realloc(a, sizeof(Pos) * (size_t)cap);
                if (tmp == NULL){
                    free(a);
                    return STATUS_ERR_ALLOC;
                }
                a = tmp;
            }
            a[k].line = line;
            a[k].pos = pos;
            k++;
        }

        if (t[i] == '\n'){
            line++;
            pos = 1;
        } else {
            pos++;
        }
    }

    *arr = a;
    *cnt = k;
    return STATUS_OK;
}

void free_res(FileRes *r, int cnt) {
    if (r == NULL) return;
    for (int i = 0; i < cnt; i++) free(r[i].arr);
    free(r);
}

// core function [pointer to the result-structure, patter, number of files)
status_t find_all(FileRes **res, const char *sub, int cnt, ...) {
    if (res == NULL || sub == NULL) return STATUS_ERR_NULL;
    if (sub[0] == '\0') return STATUS_ERR_EMPTY;
    if (check_ascii(sub, len_s(sub)) != STATUS_OK) return STATUS_ERR_NOT_ASCII;
    if (cnt < 1) return STATUS_ERR_CNT;

    FileRes *r = (FileRes *) calloc((size_t) cnt, sizeof(FileRes));
    if (r == NULL) return STATUS_ERR_ALLOC;

    va_list ap;
    va_start(ap, cnt);

    for (int i = 0; i < cnt; i++) {
        const char *path = va_arg(ap, const char*);
        r[i].path = path;

        char *text = NULL;
        size_t n = 0;
        status_t s = read_file(path, &text, &n);
        if (s == STATUS_OK) {
            s = check_ascii(text, n);
            if (s == STATUS_OK)
                s = search_text(text, n, sub, &r[i].arr, &r[i].cnt);
            free(text);
        }

        if (s != STATUS_OK) {
            va_end(ap);
            free_res(r, cnt);
            return s;
        }
    }

    va_end(ap);
    *res = r;
    return STATUS_OK;
}


// output
void print_error(status_t s){
    switch (s){
        case STATUS_ERR_NULL:  printf("Error: NULL pointer\n"); break;
        case STATUS_ERR_EMPTY: printf("Error: empty substring\n"); break;
        case STATUS_ERR_CNT:   printf("Error: count of files must be >= 1\n"); break;
        case STATUS_ERR_FILE:  printf("Error: cannot open file\n"); break;
        case STATUS_ERR_READ:  printf("Error: cannot read file\n"); break;
        case STATUS_ERR_NOT_ASCII: printf("Error: only latin letters, digits and ASCII symbols are allowed\n"); break;
        case STATUS_ERR_ALLOC: printf("Error: failed to allocate memory\n"); break;
        default:               printf("Error: unknown error\n"); break;
    }
}

// format: file: (line, pos) (line, pos) ...
void print_res(status_t s, const FileRes* r, int cnt){
    if (s != STATUS_OK){
        print_error(s);
        return;
    }
    for (int i = 0; i < cnt; i++){
        printf("%s:", r[i].path);
        if (r[i].cnt == 0) printf(" not found");
        for (int j = 0; j < r[i].cnt; j++)
            printf(" (%d, %d)", r[i].arr[j].line, r[i].arr[j].pos);
        printf("\n");
    }
}

#endif