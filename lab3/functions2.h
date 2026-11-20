#ifndef FUNCTIONS2_H
#define FUNCTIONS2_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>


#define MAX_TOKEN_LEN 10
#define TOKEN_SIZE (MAX_TOKEN_LEN + 2)
#define MINUTES_PER_DAY 1440


typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,
    STATUS_ERR_ALLOC,
    STATUS_ERR_INPUT,
    STATUS_ERR_DUPLICATE,
    STATUS_ERR_EOF
} status_t;

typedef struct {
    int id;
    int start;
    int finish;
} Talk;

const char* status_message(status_t st){
    switch (st){
        case STATUS_OK:            return "ok";
        case STATUS_ERR_NULL_PTR:  return "null pointer";
        case STATUS_ERR_ALLOC:     return "memory allocation failed";
        case STATUS_ERR_INPUT:     return "wrong input format";
        case STATUS_ERR_DUPLICATE: return "duplicate talk id";
        case STATUS_ERR_EOF:       return "unexpected end of input";
    }
    return "unknown error";
}

// HH:MM -> minutes
status_t parse_time(const char* s, int* minutes){
    if (s == NULL || minutes == NULL) return STATUS_ERR_NULL_PTR;

    if (strlen(s) != 5 || s[2] != ':') return STATUS_ERR_INPUT;

    for (int i = 0; i < 5; i++)
        if (i != 2 && (s[i] < '0' || s[i] > '9')) return STATUS_ERR_INPUT;

    int h = (s[0] - '0') * 10 + (s[1] - '0');
    int m = (s[3] - '0') * 10 + (s[4] - '0');
    if (h > 23 || m > 59)
        return STATUS_ERR_INPUT;
    *minutes = h * 60 + m;
    return STATUS_OK;
}

status_t parse_positive(const char* s, int* value){
    if (s == NULL || value == NULL) return STATUS_ERR_NULL_PTR;

    if (*s == '\0' || strlen(s) > MAX_TOKEN_LEN) return STATUS_ERR_INPUT;

    long long v = 0;
    for (const char* p = s; *p; p++){
        if (*p < '0' || *p > '9') return STATUS_ERR_INPUT;
        v = v * 10 + (*p - '0');
        if (v > INT_MAX) return STATUS_ERR_INPUT;
    }

    if (v <= 0) return STATUS_ERR_INPUT;
    *value = (int)v;
    return STATUS_OK;
}

status_t read_token(FILE* in, char* buf){
    if (fscanf(in, "%11s", buf) != 1)
        return STATUS_ERR_EOF;
    return STATUS_OK;
}

int talk_has_id(const Talk* talks, int count, int id){
    for (int i = 0; i < count; i++)
        if (talks[i].id == id)
            return 1;
    return 0;
}

status_t read_talk(FILE* in, Talk* t){
    char tok[TOKEN_SIZE];
    status_t st = read_token(in, tok);
    if (st != STATUS_OK) return st;
    st = parse_positive(tok, &t->id);
    if (st != STATUS_OK) return st;

    st = read_token(in, tok);
    if (st != STATUS_OK) return st;
    st = parse_time(tok, &t->start);
    if (st != STATUS_OK) return st;

    st = read_token(in, tok);
    if (st != STATUS_OK) return st;
    st = parse_time(tok, &t->finish);
    if (st != STATUS_OK) return st;

    if (t->start >= t->finish)
        return STATUS_ERR_INPUT;
    return STATUS_OK;
}

status_t read_input(FILE* in, Talk** talks, int* n, int* k){
    if (in == NULL || talks == NULL || n == NULL || k == NULL)
        return STATUS_ERR_NULL_PTR;
    *talks = NULL;
    char tok[TOKEN_SIZE];
    status_t st = read_token(in, tok);
    if (st != STATUS_OK) return st;

    if (strcmp(tok, "0") == 0) *n = 0;
    else{
        st = parse_positive(tok, n);
        if (st != STATUS_OK) return st;
    }

    st = read_token(in, tok);
    if (st != STATUS_OK) return st;
    st = parse_positive(tok, k);
    if (st != STATUS_OK) return st;

    if (*n == 0)
        return STATUS_OK;
    if ((size_t)*n > SIZE_MAX / sizeof(Talk))
        return STATUS_ERR_ALLOC;

    Talk* arr = (Talk*)malloc((size_t)*n * sizeof(Talk));
    if (arr == NULL)
        return STATUS_ERR_ALLOC;
    for (int i = 0; i < *n; i++){
        st = read_talk(in, &arr[i]);
        if (st == STATUS_OK && talk_has_id(arr, i, arr[i].id))
            st = STATUS_ERR_DUPLICATE;
        if (st != STATUS_OK){
            free(arr);
            return st;
        }
    }

    *talks = arr;
    return STATUS_OK;
}

int cmp_by_finish(const void* a, const void* b){
    const Talk* x = (const Talk*)a;
    const Talk* y = (const Talk*)b;
    if (x->finish != y->finish) return x->finish < y->finish ? -1 : 1;
    if (x->start != y->start)   return x->start  < y->start  ? -1 : 1;
    return (x->id > y->id) - (x->id < y->id);
}

// greedy: sort by finish, put a talk into the latest-freed hall that is free by its start
status_t max_talks(const Talk* talks, int n, int k, int* result){
    if (result == NULL || (talks == NULL && n > 0))
        return STATUS_ERR_NULL_PTR;
    if (n < 0 || k <= 0)
        return STATUS_ERR_INPUT;
    *result = 0;
    if (n == 0)
        return STATUS_OK;
    for (int i = 0; i < n; i++)
        if (talks[i].start < 0 || talks[i].finish > MINUTES_PER_DAY ||
            talks[i].start >= talks[i].finish)
            return STATUS_ERR_INPUT;

    int halls = k < n ? k : n;
    Talk* sorted = (Talk*)malloc((size_t)n * sizeof(Talk));
    int* free_at = (int*)calloc((size_t)halls, sizeof(int));
    if (sorted == NULL || free_at == NULL){
        free(sorted);
        free(free_at);
        return STATUS_ERR_ALLOC;
    }
    memcpy(sorted, talks, (size_t)n * sizeof(Talk));
    qsort(sorted, (size_t)n, sizeof(Talk), cmp_by_finish);;

    int count = 0;
    for (int i = 0; i < n; i++){
        int best = -1;
        for (int h = 0; h < halls; h++)
            if (free_at[h] <= sorted[i].start && (best < 0 || free_at[h] > free_at[best]))
                best = h;
        if (best >= 0){
            free_at[best] = sorted[i].finish;
            count++;
        }
    }
    free(sorted);
    free(free_at);
    *result = count;
    return STATUS_OK;
}

#endif