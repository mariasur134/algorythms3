#ifndef FUNCTIONS5_H
#define FUNCTIONS5_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define EXAMS 5             // number of exams
#define GRADE_MIN 0         // allowed range of a grade
#define GRADE_MAX 5
#define FIELDS (4 + EXAMS)  // id, name, surname, group + grades
#define LINE_SIZE 256       // size of a line in the dialog

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL,
    STATUS_ERR_ALLOC,
    STATUS_ERR_FORMAT,      // wrong line in the students file
    STATUS_ERR_DUPLICATE,   // two students have the same id
    STATUS_ERR_EMPTY,       // no students in the file
    STATUS_ERR_INPUT,       // wrong value in the dialog
    STATUS_ERR_EOF,
    STATUS_ERR_SAME_FILE,   // students file and trace file are the same
    STATUS_ERR_OPEN_IN,     // cannot open students file
    STATUS_ERR_OPEN_OUT,    // cannot open trace file
    STATUS_ERR_READ,        // read error
    STATUS_ERR_WRITE        // write error
} status_t;

typedef struct {
    unsigned int id;
    char* name;                 // not empty, latin letters only
    char* surname;              // not empty, latin letters only
    char* group;                // not empty
    unsigned char* grades;      // dynamic array, EXAMS elements
} Student;

typedef enum {
    FIELD_ID,
    FIELD_SURNAME,
    FIELD_NAME,
    FIELD_GROUP
} field_t;

typedef int (*comparator_t)(const void*, const void*);



// all pointers are NULL, so student_destroy is always safe
void student_init(Student* s){
    memset(s, 0, sizeof(Student));
}

void student_destroy(Student* s){
    if (s == NULL)
        return;
    free(s->name);
    free(s->surname);
    free(s->group);
    free(s->grades);
    student_init(s);
}

void students_destroy(Student* arr, size_t n){
    for (size_t i = 0; i < n; i++)
        student_destroy(&arr[i]);
    free(arr);
}

// copy of a string
char* str_dup(const char* s){
    size_t len = strlen(s) + 1;
    char* copy = (char*)malloc(len);
    if (copy != NULL)
        memcpy(copy, s, len);
    return copy;
}


// validation of all sorts
int is_blank(const char* s){
    for (; *s != '\0'; s++)
        if (*s != ' ' && *s != '\t')
            return 0;
    return 1;
}

int is_latin_word(const char* s){
    if (*s == '\0')
        return 0;
    for (; *s != '\0'; s++)
        if (!((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z')))
            return 0;
    return 1;
}

int is_group(const char* s){
    if (*s == '\0')
        return 0;
    for (; *s != '\0'; s++)
        if (*s == ' ' || *s == '\t')
            return 0;
    return 1;
}

// non-negative number: only digits, fits into unsigned int
status_t parse_id(const char* s, unsigned int* result){
    if (s == NULL || result == NULL)
        return STATUS_ERR_NULL;
    size_t len = strlen(s);
    if (len == 0 || len > 10)
        return STATUS_ERR_FORMAT;
    unsigned long long v = 0;
    for (size_t i = 0; i < len; i++){
        if (!isdigit((unsigned char)s[i]))
            return STATUS_ERR_FORMAT;
        v = v * 10 + (unsigned long long)(s[i] - '0');
    }
    if (v > UINT_MAX)
        return STATUS_ERR_FORMAT;
    *result = (unsigned int)v;
    return STATUS_OK;
}

// grade: one digit in [GRADE_MIN, GRADE_MAX]
status_t parse_grade(const char* s, unsigned char* result){
    if (s[0] < '0' || s[0] > '9' || s[1] != '\0')
        return STATUS_ERR_FORMAT;
    int v = s[0] - '0';
    if (v < GRADE_MIN || v > GRADE_MAX)
        return STATUS_ERR_FORMAT;
    *result = (unsigned char)v;
    return STATUS_OK;
}


// reading per line

status_t read_file_line(FILE* f, char** line){
    size_t cap = 64, len = 0;
    int c;
    char* buf = (char*)malloc(cap);
    if (buf == NULL)
        return STATUS_ERR_ALLOC;

    while ((c = fgetc(f)) != EOF && c != '\n'){
        if (len + 1 >= cap){
            char* tmp = (char*)realloc(buf, cap * 2);   // not into buf: keep the old pointer if it fails
            if (tmp == NULL){
                free(buf);
                return STATUS_ERR_ALLOC;
            }
            buf = tmp;
            cap *= 2;
        }
        buf[len++] = (char)c;
    }
    if (c == EOF && (len == 0 || ferror(f))){
        free(buf);
        return ferror(f) ? STATUS_ERR_READ : STATUS_ERR_EOF;
    }
    if (len > 0 && buf[len - 1] == '\r')
        len--;
    buf[len] = '\0';
    *line = buf;
    return STATUS_OK;
}

// "id name surname group g1 g2 g3 g4 g5"; the line is changed (cut into tokens)
status_t parse_student(char* line, Student* s){
    char* tok[FIELDS];
    char* p = line;

    student_init(s);
    // going through each word, remembering the start, changing ' ' to '\0'
    // 7 ivan petrv -> 7\0ivan\0petrv
    for (int i = 0; i < FIELDS; i++){
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '\0')
            return STATUS_ERR_FORMAT;       // not enough feilds
        tok[i] = p;
        while (*p != '\0' && *p != ' ' && *p != '\t') p++; // skipping the word
        if (*p != '\0')
            *p++ = '\0'; // changing the ' ' to '\0'
    }
    if (!is_blank(p))
        return STATUS_ERR_FORMAT;           // too many fields

    unsigned int id;
    unsigned char g[EXAMS];
    status_t st = parse_id(tok[0], &id);
    if (st != STATUS_OK)
        return st;
    if (!is_latin_word(tok[1]) || !is_latin_word(tok[2]) || !is_group(tok[3]))
        return STATUS_ERR_FORMAT;
    for (int i = 0; i < EXAMS; i++){
        st = parse_grade(tok[4 + i], &g[i]);
        if (st != STATUS_OK)
            return st;
    }

    s->id = id;
    s->name = str_dup(tok[1]);
    s->surname = str_dup(tok[2]);
    s->group = str_dup(tok[3]);
    s->grades = (unsigned char*)malloc(EXAMS);
    if (s->name == NULL || s->surname == NULL || s->group == NULL || s->grades == NULL){
        student_destroy(s);
        return STATUS_ERR_ALLOC;
    }
    memcpy(s->grades, g, EXAMS);
    return STATUS_OK;
}

int id_exists(const Student* arr, size_t n, unsigned int id){
    if (arr == NULL)
        return 0;
    for (size_t i = 0; i < n; i++)
        if (arr[i].id == id)
            return 1;
    return 0;
}


status_t read_students(FILE* f, Student** result, size_t* count, size_t* bad_line){
    if (f == NULL || result == NULL || count == NULL || bad_line == NULL)
        return STATUS_ERR_NULL;

    Student* arr = NULL;
    size_t n = 0, cap = 0, line_no = 0;
    status_t st = STATUS_OK;
    *bad_line = 0;

    while (st == STATUS_OK){
        char* line = NULL;
        st = read_file_line(f, &line);
        if (st == STATUS_ERR_EOF){
            st = STATUS_OK;
            break;
        }
        if (st != STATUS_OK)
            break;
        line_no++;
        if (is_blank(line)){                // empty lines are skipped
            free(line);
            continue;
        }

        Student s;
        st = parse_student(line, &s);
        free(line);
        if (st == STATUS_OK && id_exists(arr, n, s.id)){
            student_destroy(&s);
            st = STATUS_ERR_DUPLICATE;
        }
        if (st == STATUS_OK && n == cap){
            size_t new_cap = (cap == 0) ? 8 : cap * 2;
            Student* tmp = (Student*)realloc(arr, new_cap * sizeof(Student));
            if (tmp == NULL){
                student_destroy(&s);
                st = STATUS_ERR_ALLOC;
            } else {
                arr = tmp;
                cap = new_cap;
            }
        }
        if (st == STATUS_OK)
            arr[n++] = s;
        else if (st == STATUS_ERR_FORMAT || st == STATUS_ERR_DUPLICATE)
            *bad_line = line_no;
    }

    if (st == STATUS_OK && n == 0)
        st = STATUS_ERR_EMPTY;
    if (st != STATUS_OK){
        students_destroy(arr, n);
        return st;
    }
    *result = arr;
    *count = n;
    return STATUS_OK;
}


// comparison: a<b -> -1; a=b -> 0; a>b ->1;

int compare_by_id(const void* a, const void* b){
    unsigned int x = ((const Student*)a)->id;
    unsigned int y = ((const Student*)b)->id;
    return (x > y) - (x < y);
}
// ids are unique, so use second key
int compare_by_surname(const void* a, const void* b){
    int r = strcmp(((const Student*)a)->surname, ((const Student*)b)->surname);
    return (r != 0) ? r : compare_by_id(a, b);
}

int compare_by_name(const void* a, const void* b){
    int r = strcmp(((const Student*)a)->name, ((const Student*)b)->name);
    return r != 0 ? r : compare_by_id(a, b);
}

int compare_by_group(const void* a, const void* b){
    int r = strcmp(((const Student*)a)->group, ((const Student*)b)->group);
    return r != 0 ? r : compare_by_id(a, b);
}

status_t sort_students(Student* arr, size_t n, comparator_t cmp){
    if (arr == NULL || cmp == NULL)
        return STATUS_ERR_NULL;
    qsort(arr, n, sizeof(Student), cmp);
    return STATUS_OK;
}


// search
status_t find_students(const Student* arr, size_t n, field_t field, const char* key,
                       unsigned int id, size_t** idx, size_t* count){
    if (arr == NULL || idx == NULL || count == NULL || (field != FIELD_ID && key == NULL))
        return STATUS_ERR_NULL;

    *idx = NULL;
    *count = 0;
    if (n == 0)
        return STATUS_OK;

    size_t* res = (size_t*)malloc(n * sizeof(size_t));
    size_t k = 0;
    if (res == NULL)
        return STATUS_ERR_ALLOC;

    for (size_t i = 0; i < n; i++){
        int match = 0;
        switch (field){
            case FIELD_ID:      match = (arr[i].id == id); break;
            case FIELD_SURNAME: match = (strcmp(arr[i].surname, key) == 0); break;
            case FIELD_NAME:    match = (strcmp(arr[i].name, key) == 0); break;
            case FIELD_GROUP:   match = (strcmp(arr[i].group, key) == 0); break;
        }
        if (match)
            res[k++] = i;
    }
    *idx = res;
    *count = k;
    return STATUS_OK;
}


// average marks

unsigned int student_sum(const Student* s){
    unsigned int sum = 0;
    for (int i = 0; i < EXAMS; i++)
        sum += s->grades[i];
    return sum;
}

double student_avg(const Student* s){
    return (double)student_sum(s) / EXAMS;
}


status_t find_above_average(const Student* arr, size_t n, size_t** idx, size_t* count){
    if (arr == NULL || idx == NULL || count == NULL)
        return STATUS_ERR_NULL;

    *idx = NULL;
    *count = 0;
    if (n == 0)
        return STATUS_OK;

    size_t* res = (size_t*)malloc(n * sizeof(size_t));
    size_t k = 0;
    unsigned long long total = 0;
    if (res == NULL)
        return STATUS_ERR_ALLOC;

    for (size_t i = 0; i < n; i++)
        total += student_sum(&arr[i]);
    for (size_t i = 0; i < n; i++)
        if ((unsigned long long)student_sum(&arr[i]) * n > total)
            res[k++] = i;
    *idx = res;
    *count = k;
    return STATUS_OK;
}


// output

status_t write_student(FILE* out, const Student* s){
    if (out == NULL || s == NULL)
        return STATUS_ERR_NULL;
    if (fprintf(out, "id=%u  %s %s  group %s  average %.2f\n",
                s->id, s->surname, s->name, s->group, student_avg(s)) < 0)
        return STATUS_ERR_WRITE;
    return STATUS_OK;
}

status_t write_students(FILE* out, const Student* arr, const size_t* idx, size_t count){
    status_t st = STATUS_OK;
    for (size_t i = 0; i < count && st == STATUS_OK; i++)
        st = write_student(out, &arr[idx != NULL ? idx[i] : i]);
    return st;
}

status_t write_names(FILE* out, const Student* arr, const size_t* idx, size_t count){
    if (out == NULL || arr == NULL || (idx == NULL && count > 0))
        return STATUS_ERR_NULL;
    for (size_t i = 0; i < count; i++)
        if (fprintf(out, "%s %s\n", arr[idx[i]].surname, arr[idx[i]].name) < 0)
            return STATUS_ERR_WRITE;
    return STATUS_OK;
}


void print_status(status_t st, size_t bad_line){
    switch (st){
        case STATUS_OK:             break;
        case STATUS_ERR_NULL:       printf("Error: NULL pointer\n"); break;
        case STATUS_ERR_ALLOC:      printf("Error: failed to allocate memory\n"); break;
        case STATUS_ERR_FORMAT:
            printf("Error: wrong data in line %zu (expected: id name surname group and %d grades %d..%d)\n",
                   bad_line, EXAMS, GRADE_MIN, GRADE_MAX);
            break;
        case STATUS_ERR_DUPLICATE:  printf("Error: line %zu has an id that already exists\n", bad_line); break;
        case STATUS_ERR_EMPTY:      printf("Error: no students in the file\n"); break;
        case STATUS_ERR_INPUT:      printf("Error: wrong value\n"); break;
        case STATUS_ERR_EOF:        printf("Error: input ended\n"); break;
        case STATUS_ERR_SAME_FILE:  printf("Error: students file and trace file must be different\n"); break;
        case STATUS_ERR_OPEN_IN:    printf("Error: cannot open students file\n"); break;
        case STATUS_ERR_OPEN_OUT:   printf("Error: cannot open trace file\n"); break;
        case STATUS_ERR_READ:       printf("Error: cannot read students file\n"); break;
        case STATUS_ERR_WRITE:      printf("Error: cannot write trace file\n"); break;
        default:                    printf("Error: unknown error\n"); break;
    }
}
#endif
