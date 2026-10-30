#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>

// tolerance for comparing norms and for the matrix checks
#ifndef EPS
#define EPS 1e-9
#endif

typedef enum {
    STATUS_OK,           // ok
    STATUS_ERR_NULL,     // NULL pointer
    STATUS_ERR_DIM,      // n < 1
    STATUS_ERR_CNT,      // number of vectors / norms < 1
    STATUS_ERR_P,        // p < 1 or p is not finite
    STATUS_ERR_MATRIX,   // matrix is not symmetric positive definite
    STATUS_ERR_NUMERIC,  // NaN / inf in coordinates or in a norm
    STATUS_ERR_ALLOC     // malloc failed
} status_t;



typedef struct {
    const double* coords;
} vector_t;


typedef status_t (*norm_fn)(const double* x, int n, const void* params, double* out);

typedef struct {
    const double* a;
    int n;
} matrix_params_t;

// result for one norm
typedef struct {
    double max_norm;   // the biggest norm
    int count;         // how many vectors have it
    int* idx;          // their indixes
} norm_result_t;



int is_equal(double a, double b){
    double scale = 1.0;
    if (fabs(a) > scale) scale = fabs(a);
    if (fabs(b) > scale) scale = fabs(b);
    return fabs(a - b) <= EPS * scale;
}

void free_results(norm_result_t* r, int cnt){
    if (r == NULL) return;
    for (int k = 0; k < cnt; k++)
        free(r[k].idx);
    free(r);
}



// 1st - inf
status_t norm_inf(const double* x, int n, const void* params, double* out){
    (void)params;
    if (x == NULL || out == NULL) return STATUS_ERR_NULL;
    if (n < 1) return STATUS_ERR_DIM;

    double m = 0.0;
    for (int j = 0; j < n; j++){
        double a = fabs(x[j]);
        if (a > m) m = a;
    }
    *out = m;
    return STATUS_OK;
}

// 2nd - summ
status_t norm_p(const double* x, int n, const void* params, double* out){
    if (x == NULL || params == NULL || out == NULL) return STATUS_ERR_NULL;
    if (n < 1) return STATUS_ERR_DIM;

    double p = *(const double*)params; // getting par p

    if (!(p >= 1.0) || !isfinite(p)) return STATUS_ERR_P;

    double m = 0.0;
    for (int j = 0; j < n; j++){
        double a = fabs(x[j]);
        if (a > m) m = a;
    }
    if (m == 0.0){
        *out = 0.0;
        return STATUS_OK;
    }

    double s = 0.0;
    for (int j = 0; j < n; j++)
        s += pow(fabs(x[j]) / m, p);

    *out = m * pow(s, 1.0 / p);
    return STATUS_OK;
}

// 3d - using matrix (x^tAx)
status_t norm_a(const double* x, int n, const void* params, double* out){
    if (x == NULL || params == NULL || out == NULL) return STATUS_ERR_NULL;

    const matrix_params_t* mp = (const matrix_params_t*)params;
    if (mp->a == NULL) return STATUS_ERR_NULL;
    if (n < 1 || mp->n != n) return STATUS_ERR_DIM;

    double q = 0.0;
    for (int i = 0; i < n; i++){
        double row = 0.0;
        for (int j = 0; j < n; j++)
            row += mp->a[(size_t)i * (size_t)n + (size_t)j] * x[j];
        q += x[i] * row;
    }
    if (q < 0.0) q = 0.0;

    *out = sqrt(q);
    return STATUS_OK;
}


status_t init_matrix_params(matrix_params_t* mp, const double* a, int n){
    if (mp == NULL || a == NULL) return STATUS_ERR_NULL;
    if (n < 1) return STATUS_ERR_DIM;
    if ((size_t)n > SIZE_MAX / (size_t)n / sizeof(double)) return STATUS_ERR_ALLOC;

    size_t nn = (size_t)n;

    // symmetry
    for (size_t i = 0; i < nn; i++)
        for (size_t j = 0; j < i; j++)
            if (!is_equal(a[i * nn + j], a[j * nn + i])) return STATUS_ERR_MATRIX;

    // A = LL^t
    double* l = (double*)calloc(nn * nn, sizeof(double));
    if (l == NULL) return STATUS_ERR_ALLOC;

    for (size_t i = 0; i < nn; i++){
        for (size_t j = 0; j <= i; j++){
            double s = a[i * nn + j];
            for (size_t k = 0; k < j; k++)
                s -= l[i * nn + k] * l[j * nn + k];

            if (i == j){
                if (!(s > EPS * fabs(a[i * nn + i]))){
                    free(l);
                    return STATUS_ERR_MATRIX;
                }
                l[i * nn + i] = sqrt(s);
            } else {
                l[i * nn + j] = s / l[j * nn + j];
            }
        }
    }

    free(l);
    mp->a = a;
    mp->n = n;
    return STATUS_OK;
}



status_t process_norm(const norm_fn f, const void* params, const vector_t* vecs,
                      int vec_cnt, int n, double* vals, norm_result_t* r){
    double mx = 0.0;
    for (int i = 0; i < vec_cnt; i++){
        status_t s = f(vecs[i].coords, n, params, &vals[i]);
        if (s != STATUS_OK) return s;
        if (!isfinite(vals[i])) return STATUS_ERR_NUMERIC;
        if (i == 0 || vals[i] > mx) mx = vals[i];
    }

    int cnt = 0;
    for (int i = 0; i < vec_cnt; i++)
        if (is_equal(vals[i], mx)) cnt++;

    int* idx = (int*)malloc(sizeof(int) * (size_t)cnt);
    if (idx == NULL) return STATUS_ERR_ALLOC;

    int k = 0;
    for (int i = 0; i < vec_cnt; i++)
        if (is_equal(vals[i], mx)) idx[k++] = i;

    r->max_norm = mx;
    r->count = cnt;
    r->idx = idx;
    return STATUS_OK;
}

void release(vector_t* vecs, double* vals, norm_result_t* out, int norm_cnt){
    free(vecs);
    free(vals);
    free_results(out, norm_cnt);
}


status_t longest_vectors(norm_result_t** res, int n, int vec_cnt, int norm_cnt, ...){
    if (res == NULL) return STATUS_ERR_NULL;
    if (n < 1) return STATUS_ERR_DIM;
    if (vec_cnt < 1 || norm_cnt < 1) return STATUS_ERR_CNT;

    vector_t* vecs = (vector_t*)malloc(sizeof(vector_t) * (size_t)vec_cnt); // copied vectors
    double* vals = (double*)malloc(sizeof(double) * (size_t)vec_cnt); // returned vals
    norm_result_t* out = (norm_result_t*)calloc((size_t)norm_cnt, sizeof(norm_result_t)); //results

    if (vecs == NULL || vals == NULL || out == NULL){
        release(vecs, vals, out, norm_cnt);
        return STATUS_ERR_ALLOC;
    }

    va_list ap;
    va_start(ap, norm_cnt);

    for (int i = 0; i < vec_cnt; i++){
        vecs[i] = va_arg(ap, vector_t);

        if (vecs[i].coords == NULL){
            va_end(ap);
            release(vecs, vals, out, norm_cnt);
            return STATUS_ERR_NULL;
        }

        for (int j = 0; j < n; j++){
            double c = vecs[i].coords[j];
            if (!isfinite(c)){
                va_end(ap);
                release(vecs, vals, out, norm_cnt);
                return STATUS_ERR_NUMERIC;
            }
        }
    }

    for (int k = 0; k < norm_cnt; k++){
        norm_fn f = va_arg(ap, norm_fn);
        const void* params = va_arg(ap, const void*);

        status_t s = STATUS_ERR_NULL;
        if (f != NULL)
            s = process_norm(f, params, vecs, vec_cnt, n, vals, &out[k]);

        if (s != STATUS_OK){
            va_end(ap);
            release(vecs, vals, out, norm_cnt);
            return s;
        }
    }

    va_end(ap);
    free(vecs);
    free(vals);
    *res = out;
    return STATUS_OK;
}

void print_error(status_t s){
    switch (s){
        case STATUS_ERR_NULL:    printf("Error: NULL pointer\n"); break;
        case STATUS_ERR_DIM:     printf("Error: wrong dimension\n"); break;
        case STATUS_ERR_CNT:     printf("Error: count of vectors and norms must be >= 1\n"); break;
        case STATUS_ERR_P:       printf("Error: p must be a finite number >= 1\n"); break;
        case STATUS_ERR_MATRIX:  printf("Error: matrix must be symmetric positive definite\n"); break;
        case STATUS_ERR_NUMERIC: printf("Error: NaN or inf in coordinates or in a norm\n"); break;
        case STATUS_ERR_ALLOC:   printf("Error: failed to allocate memory\n"); break;
        default:                 printf("Error: unknown error\n"); break;
    }
}

void print_vector(const double* x, int n){
    printf("(");
    for (int j = 0; j < n; j++)
        printf("%g%s", x[j], (j < n - 1) ? ", " : "");
    printf(")");
}

void print_results(status_t s, const vector_t* vecs, int n,
                   const char* const* names, const norm_result_t* res, int norm_cnt)
{
    if (s != STATUS_OK){
        print_error(s);
        return;
    }
    for (int k = 0; k < norm_cnt; k++){
        printf("%s: max norm = %g\n", names[k], res[k].max_norm);
        for (int i = 0; i < res[k].count; i++){
            int id = res[k].idx[i];
            printf("  vector #%d ", id + 1);
            print_vector(vecs[id].coords, n);
            printf("\n");
        }
    }
}

#endif