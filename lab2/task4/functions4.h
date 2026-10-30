#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    STATUS_OK,          // ok
    STATUS_ERR_NULL,    // NULL pointer
    STATUS_ERR_CNT,     // n < 1
    STATUS_ERR_ALLOC,   // malloc/calloc failed
    STATUS_ERR_OVERFLOW // n * (capacity+1) overflows size_t
} status_t;


// description of one file
typedef struct {
    char* name;                  // dynamically allocated
    unsigned long long size;     // > 0
    unsigned long long importance;
} file_desc_t;


// result
typedef struct {
    file_desc_t* files;          // array of chosen files
    int count;                   // number of chosen files
    unsigned long long total_size;
    unsigned long long total_importance;
} backup_result_t;

void free_result(backup_result_t* r){
    if (r == NULL) return;
    free(r->files);
    r->files = NULL;
    r->count = 0;
    r->total_size = 0;
    r->total_importance = 0;
}


status_t select_files(const file_desc_t* files, int n,
                      unsigned long long cap, backup_result_t* result)
{
    if (files == NULL || result == NULL) return STATUS_ERR_NULL;
    if (n < 1) return STATUS_ERR_CNT;

    // init result
    result->files = NULL;
    result->count = 0;
    result->total_size = 0;
    result->total_importance = 0;


    if (cap > (size_t)-1 / sizeof(unsigned long long) - 1)
        return STATUS_ERR_OVERFLOW;

    size_t w_cnt = (size_t)(cap + 1);

    // check overflow
    if (w_cnt > 0 && (size_t)n > (size_t)-1 / w_cnt)
        return STATUS_ERR_OVERFLOW;

    // dp[w] = highes imp with size taken <= w
    // sz[w] = size of that best choice
    unsigned long long* dp = (unsigned long long*)calloc(w_cnt, sizeof(unsigned long long));
    unsigned long long* sz = (unsigned long long*)calloc(w_cnt, sizeof(unsigned long long));
    if (dp == NULL || sz == NULL){
        free(dp);
        free(sz);
        return STATUS_ERR_ALLOC;
    }

    // take[i * w_cnt + w] = 1 if
    // [file i is taken at capacity w]
    unsigned char* take = (unsigned char*)calloc((size_t)n * w_cnt, sizeof(unsigned char));
    if (take == NULL){
        free(dp);
        free(sz);
        return STATUS_ERR_ALLOC;
    }

    // fill DP
    for (int i = 0; i < n; i++){
        unsigned long long fi_size = files[i].size;
        unsigned long long fi_imp  = files[i].importance;

        // iterate w from cap down to fi_size
        // (unsigned: w >= fi_size && w <= cap)
        if (fi_size > cap) continue;

        for (unsigned long long w = cap; ; w--){
            unsigned long long prev_w = w - fi_size;
            unsigned long long new_imp = dp[prev_w] + fi_imp;
            unsigned long long new_sz  = sz[prev_w] + fi_size;

            // take if: strictly better importance,
            // or same importance but smaller total size
            if (new_imp > dp[w] || (new_imp == dp[w] && new_sz < sz[w])){
                dp[w] = new_imp;
                sz[w] = new_sz;
                take[i * w_cnt + w] = 1;
            }

            if (w == fi_size) break;
        }
    }

    // count chosen files
    {
        unsigned long long w = cap;
        int cnt = 0;
        for (int i = n - 1; i >= 0; i--){
            if (take[i * w_cnt + w]){
                cnt++;
                w -= files[i].size;
            }
        }

        if (cnt > 0){
            file_desc_t* chosen = (file_desc_t*)malloc(sizeof(file_desc_t) * (size_t)cnt);
            if (chosen == NULL){
                free(dp); free(sz); free(take);
                return STATUS_ERR_ALLOC;
            }

            w = cap;
            // first mark which files are chosen
            int* marks = (int*)calloc((size_t)n, sizeof(int));
            if (marks == NULL){
                free(dp); free(sz); free(take); free(chosen);
                return STATUS_ERR_ALLOC;
            }
            for (int i = n - 1; i >= 0; i--){
                if (take[i * w_cnt + w]){
                    marks[i] = 1;
                    w -= files[i].size;
                }
            }

            int idx = 0;
            unsigned long long ts = 0, ti = 0;
            for (int i = 0; i < n; i++){
                if (marks[i]){
                    chosen[idx].name       = files[i].name; // pointer to original
                    chosen[idx].size       = files[i].size;
                    chosen[idx].importance = files[i].importance;
                    ts += files[i].size;
                    ti += files[i].importance;
                    idx++;
                }
            }

            result->files            = chosen;
            result->count            = cnt;
            result->total_size       = ts;
            result->total_importance = ti;

            free(marks);
        }
    }

    free(dp);
    free(sz);
    free(take);
    return STATUS_OK;
}


// output
void print_error(status_t s){
    switch (s){
        case STATUS_ERR_NULL:     printf("Error: NULL pointer\n"); break;
        case STATUS_ERR_CNT:      printf("Error: n must be >= 1\n"); break;
        case STATUS_ERR_ALLOC:    printf("Error: failed to allocate memory\n"); break;
        case STATUS_ERR_OVERFLOW: printf("Error: capacity too large for available memory\n"); break;
        default:                  printf("Error: unknown error\n"); break;
    }
}

void print_result(status_t s, const backup_result_t* r){
    if (s != STATUS_OK){
        print_error(s);
        return;
    }
    printf("TOTAL_SIZE %llu\n", r->total_size);
    printf("TOTAL_IMPORTANCE %llu\n", r->total_importance);
    printf("FILES %d\n", r->count);
    for (int i = 0; i < r->count; i++)
        printf("%s\n", r->files[i].name);
}

#endif
