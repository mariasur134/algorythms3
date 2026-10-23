#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

#ifndef EPS
#define EPS 1e-9
#endif


#define NAME_LEN 64 // buffer size for name / surname: max 63 letters + '\0'
#define LINE_LEN 256 // buffer size for one line: id (10) + 2 names (63 + 63) + salary + spaces < 256
#define START_CAP 16

typedef enum {
    STATUS_OK,              // ok
    STATUS_ERR_NULL,        // NULL pointer
    STATUS_ERR_FLAG,        // unknown flag
    STATUS_ERR_SAME_FILE,   // input and output are the same file
    STATUS_ERR_OPEN_IN,     // cannot open input file
    STATUS_ERR_OPEN_OUT,    // cannot open output file
    STATUS_ERR_READ,        // error while reading
    STATUS_ERR_WRITE,       // error while writing
    STATUS_ERR_FORMAT,      // wrong line in input file
    STATUS_ERR_ALLOC,       // malloc / realloc failed
    STATUS_END              // end of file (not an error)
} status_t;

typedef struct {
    unsigned int id;
    char name[NAME_LEN];
    char surname[NAME_LEN];
    double salary;
} Employee;


// helpers
int is_digit_c(char c){
    return c >= '0' && c <= '9';
}

int is_alpha_c(char c){
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int is_space_c(char c){
    return c == ' ' || c == '\t';
}


// id: only digits, <= UINT_MAX
status_t parse_id(const char* s, unsigned int* res){
    if (s == NULL || res == NULL) return STATUS_ERR_NULL;
    if (s[0] == '\0') return STATUS_ERR_FORMAT;

    unsigned int v = 0;
    for (size_t i = 0; s[i] != '\0'; i++){
        if (!is_digit_c(s[i])) return STATUS_ERR_FORMAT;
        unsigned int d = (unsigned int)(s[i] - '0');
        if (v > (UINT_MAX - d) / 10) return STATUS_ERR_FORMAT;
        v = v * 10 + d;
    }
    *res = v;
    return STATUS_OK;
}

// name: non-empty, only latin letters, < NAME_LEN chars; copied to out
status_t parse_name(const char* s, char* out){
    if (s == NULL || out == NULL) return STATUS_ERR_NULL;
    if (s[0] == '\0') return STATUS_ERR_FORMAT;

    size_t i = 0;
    for (; s[i] != '\0'; i++){
        if (!is_alpha_c(s[i])) return STATUS_ERR_FORMAT;   // also rejects cyrillic
        if (i + 1 >= NAME_LEN) return STATUS_ERR_FORMAT;    // too long
        out[i] = s[i];
    }
    out[i] = '\0';
    return STATUS_OK;
}

// salary: digits [ '.' digits ], e.g. 1500 or 1500.50 (no '-', ',', 'e')
status_t parse_salary(const char* s, double* res){
    if (s == NULL || res == NULL) return STATUS_ERR_NULL;

    size_t i = 0;
    if (!is_digit_c(s[i])) return STATUS_ERR_FORMAT;     // ".5" is not allowed
    while (is_digit_c(s[i])) i++;

    if (s[i] == '.'){
        i++;
        if (!is_digit_c(s[i])) return STATUS_ERR_FORMAT; // "15." is not allowed
        while (is_digit_c(s[i])) i++;
    }
    if (s[i] != '\0') return STATUS_ERR_FORMAT;

    // format is already checked, strtod only converts
    double v = strtod(s, NULL);
    if (v > DBL_MAX) return STATUS_ERR_FORMAT;          // too big (inf)

    *res = v;
    return STATUS_OK;
}


// splits line by spaces / tabs (line is modified: '\0' after each word)
// tok gets up to max words, cnt - number of words found
status_t split_line(char* line, char* tok[], int max, int* cnt){
    if (line == NULL || tok == NULL || cnt == NULL) return STATUS_ERR_NULL;

    int k = 0;
    size_t i = 0;
    while (line[i] != '\0'){
        while (is_space_c(line[i])) i++;
        if (line[i] == '\0') break;

        if (k == max){        // too many words
            *cnt = k + 1;
            return STATUS_OK;
        }
        tok[k++] = &line[i];

        while (line[i] != '\0' && !is_space_c(line[i])) i++;
        if (line[i] != '\0') line[i++] = '\0';
    }
    *cnt = k;
    return STATUS_OK;
}

// "id name surname salary" -> Employee
status_t parse_employee(char* line, Employee* e){
    if (line == NULL || e == NULL) return STATUS_ERR_NULL;

    char* tok[4];
    int cnt = 0;
    split_line(line, tok, 4, &cnt);
    if (cnt != 4) return STATUS_ERR_FORMAT;

    if (parse_id(tok[0], &e->id) != STATUS_OK) return STATUS_ERR_FORMAT;
    if (parse_name(tok[1], e->name) != STATUS_OK) return STATUS_ERR_FORMAT;
    if (parse_name(tok[2], e->surname) != STATUS_OK) return STATUS_ERR_FORMAT;
    if (parse_salary(tok[3], &e->salary) != STATUS_OK) return STATUS_ERR_FORMAT;
    return STATUS_OK;
}


// reads one line into buf (without '\n' and '\r')
// STATUS_END - file is over, STATUS_ERR_FORMAT - line is longer than size - 1
status_t read_line(FILE* in, char* buf, size_t size){
    if (in == NULL || buf == NULL) return STATUS_ERR_NULL;

    int c = fgetc(in);
    if (c == EOF) return ferror(in) ? STATUS_ERR_READ : STATUS_END;

    size_t n = 0;
    while (c != EOF && c != '\n'){
        if (c != '\r'){
            if (n + 1 >= size) return STATUS_ERR_FORMAT;
            buf[n++] = (char)c;
        }
        c = fgetc(in);
    }
    if (ferror(in)) return STATUS_ERR_READ;

    buf[n] = '\0';
    return STATUS_OK;
}

// line has only spaces / tabs
int is_blank(const char* s){
    for (size_t i = 0; s[i] != '\0'; i++)
        if (!is_space_c(s[i])) return 0;
    return 1;
}

// reads all employees into dynamic array (arr must be freed)
// on STATUS_ERR_FORMAT *bad_line is the number of wrong line (from 1)
status_t read_employees(FILE* in, Employee** arr, size_t* cnt, size_t* bad_line){
    if (in == NULL || arr == NULL || cnt == NULL || bad_line == NULL) return STATUS_ERR_NULL;

    size_t cap = START_CAP, n = 0;
    Employee* a = (Employee*)malloc(sizeof(Employee) * cap);
    if (a == NULL) return STATUS_ERR_ALLOC;

    char line[LINE_LEN];
    size_t line_no = 0;

    while (1){
        status_t s = read_line(in, line, sizeof(line));
        line_no++;
        if (s == STATUS_END) break;
        if (s != STATUS_OK){
            free(a);
            *bad_line = line_no;
            return s;
        }
        if (is_blank(line)) continue;   // empty lines are skipped

        if (n == cap){
            cap *= 2;
            Employee* t = (Employee*)realloc(a, sizeof(Employee) * cap);
            if (t == NULL){
                free(a);
                return STATUS_ERR_ALLOC;
            }
            a = t;
        }

        if (parse_employee(line, &a[n]) != STATUS_OK){
            free(a);
            *bad_line = line_no;
            return STATUS_ERR_FORMAT;
        }
        n++;
    }

    *arr = a;
    *cnt = n;
    return STATUS_OK;
}


// comparators for qsort: < 0 if a goes before b
int cmp_double(double a, double b, double eps){
    if (a - b > eps) return 1;
    if (b - a > eps) return -1;
    return 0;
}

// salary -> surname -> name -> id
int cmp_asc(const void* pa, const void* pb){
    const Employee* a = (const Employee*)pa;
    const Employee* b = (const Employee*)pb;

    int r = cmp_double(a->salary, b->salary, EPS);
    if (r != 0) return r;

    r = strcmp(a->surname, b->surname);
    if (r != 0) return r;

    r = strcmp(a->name, b->name);
    if (r != 0) return r;

    return (a->id > b->id) - (a->id < b->id);   // not a->id - b->id: unsigned
}

int cmp_desc(const void* pa, const void* pb){
    return cmp_asc(pb, pa);
}

// flag: 'a' - ascending, 'd' - descending
status_t sort_employees(Employee* arr, size_t cnt, char flag){
    if (arr == NULL && cnt > 0) return STATUS_ERR_NULL;
    if (flag != 'a' && flag != 'd') return STATUS_ERR_FLAG;
    if (cnt < 2) return STATUS_OK;

    qsort(arr, cnt, sizeof(Employee), (flag == 'a') ? cmp_asc : cmp_desc);
    return STATUS_OK;
}

status_t write_employees(FILE* out, const Employee* arr, size_t cnt){
    if (out == NULL || (arr == NULL && cnt > 0)) return STATUS_ERR_NULL;

    for (size_t i = 0; i < cnt; i++)
        if (fprintf(out, "%u %s %s %.2f\n", arr[i].id, arr[i].name,
                    arr[i].surname, arr[i].salary) < 0)
            return STATUS_ERR_WRITE;
    return STATUS_OK;
}

#endif
