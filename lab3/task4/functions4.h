#ifndef FUNCTIONS4_H
#define FUNCTIONS4_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <time.h>

#define INDEX_LEN 6          // postal index: 6 digits
#define ID_LEN 14            // mail identifier: 14 digits
#define DATETIME_LEN 19      // "dd:MM:yyyy hh:mm:ss"
#define START_CAPACITY 4

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,    // null pointer passed to function
    STATUS_ERR_ALLOC,       // malloc / realloc failed
    STATUS_ERR_INPUT,       // value has a wrong format
    STATUS_ERR_NOT_FOUND,   // no mail with such identifier
    STATUS_ERR_DUPLICATE,   // mail with such identifier already exists
    STATUS_ERR_EOF,         // input stream ended
    STATUS_ERR_TIME         // system time is not available
} status_t;

typedef enum {
    FILTER_DELIVERED,
    FILTER_EXPIRED
} filter_t;

/* ======================= Part 1: String ======================= */

typedef struct {
    char* data;     // dynamic array of chars, always ends with '\0'
    int length;     // number of chars without '\0'
} String;

// create String from a C string (copies the chars)
status_t string_create(String* s, const char* src){
    if (s == NULL || src == NULL)
        return STATUS_ERR_NULL_PTR;
    size_t len = strlen(src);
    if (len >= (size_t)INT_MAX)
        return STATUS_ERR_INPUT;
    char* buf = (char*)malloc(len + 1);
    if (buf == NULL){
        s->data = NULL;
        s->length = 0;
        return STATUS_ERR_ALLOC;
    }
    memcpy(buf, src, len + 1);
    s->data = buf;
    s->length = (int)len;
    return STATUS_OK;
}

// free the inner content; safe to call twice
void string_destroy(String* s){
    if (s == NULL)
        return;
    free(s->data);
    s->data = NULL;
    s->length = 0;
}

// lexicographic comparator: <0, 0, >0 (NULL is smaller than any string)
int string_compare_lex(const String* a, const String* b){
    if (a == NULL || a->data == NULL)
        return (b == NULL || b->data == NULL) ? 0 : -1;
    if (b == NULL || b->data == NULL)
        return 1;
    return strcmp(a->data, b->data);
}

// equivalence relation: 1 if the strings are equal, 0 otherwise
int string_equal(const String* a, const String* b){
    return string_compare_lex(a, b) == 0;
}

// order relation: by length first, then lexicographically: <0, 0, >0
int string_compare_order(const String* a, const String* b){
    if (a == NULL || a->data == NULL || b == NULL || b->data == NULL)
        return string_compare_lex(a, b);
    if (a->length != b->length)
        return (a->length < b->length) ? -1 : 1;
    return strcmp(a->data, b->data);
}

// copy src into an existing String; old content is replaced only on success
status_t string_copy(String* dst, const String* src){
    if (dst == NULL || src == NULL || src->data == NULL)
        return STATUS_ERR_NULL_PTR;
    String tmp;
    status_t st = string_create(&tmp, src->data);
    if (st != STATUS_OK)
        return st;
    string_destroy(dst);
    *dst = tmp;
    return STATUS_OK;
}

// copy src into a new String placed in the heap; free with string_free
status_t string_clone(const String* src, String** result){
    if (result == NULL)
        return STATUS_ERR_NULL_PTR;
    *result = NULL;
    if (src == NULL || src->data == NULL)
        return STATUS_ERR_NULL_PTR;
    String* s = (String*)malloc(sizeof(String));
    if (s == NULL)
        return STATUS_ERR_ALLOC;
    status_t st = string_create(s, src->data);
    if (st != STATUS_OK){
        free(s);
        return st;
    }
    *result = s;
    return STATUS_OK;
}

// free a String created by string_clone (content and the struct itself)
void string_free(String* s){
    if (s == NULL)
        return;
    string_destroy(s);
    free(s);
}

// append the content of src to dst; src may be the same object as dst
status_t string_concat(String* dst, const String* src){
    if (dst == NULL || dst->data == NULL || src == NULL || src->data == NULL)
        return STATUS_ERR_NULL_PTR;
    size_t len_dst = (size_t)dst->length;
    size_t len_src = (size_t)src->length;
    if (len_dst + len_src >= (size_t)INT_MAX)
        return STATUS_ERR_INPUT;
    char* buf = (char*)realloc(dst->data, len_dst + len_src + 1);
    if (buf == NULL)
        return STATUS_ERR_ALLOC;
    // if src == dst, its data pointer is stale after realloc: use the new buffer
    const char* from = (src == dst) ? buf : src->data;
    memcpy(buf + len_dst, from, len_src);
    buf[len_dst + len_src] = '\0';
    dst->data = buf;
    dst->length = (int)(len_dst + len_src);
    return STATUS_OK;
}

/* ======================= Part 2: Mail, Address, Post ======================= */

typedef struct {
    String city;        // not empty
    String street;      // not empty
    int house;          // natural number
    String building;    // may be empty
    int apartment;      // natural number
    String index;       // 6 digits
} Address;

typedef struct {
    Address address;        // recipient
    double weight;          // >= 0
    String id;              // 14 digits
    String created;         // "dd:MM:yyyy hh:mm:ss"
    String deadline;        // "dd:MM:yyyy hh:mm:ss", planned delivery time
    long long created_key;  // created as number yyyyMMddhhmmss (for comparing)
    long long deadline_key;
    int delivered;          // 1 if the mail was handed to the recipient
} Mail;

typedef struct {
    Address* office;    // address of the current post office
    Mail* mails;        // dynamic array, always sorted by (index, id)
    int count;
    int capacity;
} Post;

void address_destroy(Address* a){
    if (a == NULL)
        return;
    string_destroy(&a->city);
    string_destroy(&a->street);
    string_destroy(&a->building);
    string_destroy(&a->index);
}

void mail_destroy(Mail* m){
    if (m == NULL)
        return;
    address_destroy(&m->address);
    string_destroy(&m->id);
    string_destroy(&m->created);
    string_destroy(&m->deadline);
}

// zero all fields so that destroy functions are safe on a partly filled object
void address_init(Address* a){
    a->city.data = a->street.data = a->building.data = a->index.data = NULL;
    a->city.length = a->street.length = a->building.length = a->index.length = 0;
    a->house = 0;
    a->apartment = 0;
}

void mail_init(Mail* m){
    address_init(&m->address);
    m->weight = 0.0;
    m->id.data = m->created.data = m->deadline.data = NULL;
    m->id.length = m->created.length = m->deadline.length = 0;
    m->created_key = 0;
    m->deadline_key = 0;
    m->delivered = 0;
}

void post_init(Post* p){
    p->office = NULL;
    p->mails = NULL;
    p->count = 0;
    p->capacity = 0;
}

void post_destroy(Post* p){
    if (p == NULL)
        return;
    for (int i = 0; i < p->count; i++)
        mail_destroy(&p->mails[i]);
    free(p->mails);
    if (p->office != NULL){
        address_destroy(p->office);
        free(p->office);
    }
    post_init(p);
}

/* ---------- validation (no input / output here) ---------- */

// 1 if s consists of exactly count digits
int is_digits(const char* s, int count){
    if (s == NULL || (int)strlen(s) != count)
        return 0;
    for (int i = 0; i < count; i++)
        if (!isdigit((unsigned char)s[i]))
            return 0;
    return 1;
}

// natural number: only digits, value in [1, INT_MAX]
status_t parse_natural(const char* s, int* result){
    if (s == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;
    if (*s == '\0')
        return STATUS_ERR_INPUT;
    long long v = 0;
    for (const char* p = s; *p != '\0'; p++){
        if (!isdigit((unsigned char)*p))
            return STATUS_ERR_INPUT;
        v = v * 10 + (*p - '0');
        if (v > INT_MAX)
            return STATUS_ERR_INPUT;
    }
    if (v < 1)
        return STATUS_ERR_INPUT;
    *result = (int)v;
    return STATUS_OK;
}

// non-negative real number: digits with at most one '.', no sign, no exponent
status_t parse_weight(const char* s, double* result){
    if (s == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;
    int digits = 0, dots = 0;
    for (const char* p = s; *p != '\0'; p++){
        if (isdigit((unsigned char)*p))
            digits++;
        else if (*p == '.')
            dots++;
        else
            return STATUS_ERR_INPUT;
    }
    if (digits == 0 || dots > 1)
        return STATUS_ERR_INPUT;
    double v = strtod(s, NULL);
    if (!isfinite(v))
        return STATUS_ERR_INPUT;
    *result = v;
    return STATUS_OK;
}

int is_leap_year(int y){
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

int days_in_month(int y, int m){
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && is_leap_year(y))
        return 29;
    return days[m - 1];
}

// build the number yyyyMMddhhmmss, which keeps the chronological order
long long make_time_key(int y, int mo, int d, int h, int mi, int s){
    return ((((((long long)y * 100 + mo) * 100 + d) * 100 + h) * 100 + mi) * 100) + s;
}

// check "dd:MM:yyyy hh:mm:ss" and compute its time key
status_t parse_datetime(const char* s, long long* key){
    if (s == NULL || key == NULL)
        return STATUS_ERR_NULL_PTR;
    if ((int)strlen(s) != DATETIME_LEN)
        return STATUS_ERR_INPUT;
    for (int i = 0; i < DATETIME_LEN; i++){
        char c = s[i];
        if (i == 2 || i == 5 || i == 13 || i == 16){
            if (c != ':')
                return STATUS_ERR_INPUT;
        }
        else if (i == 10){
            if (c != ' ')
                return STATUS_ERR_INPUT;
        }
        else if (!isdigit((unsigned char)c))
            return STATUS_ERR_INPUT;
    }
    int d = (s[0] - '0') * 10 + (s[1] - '0');
    int mo = (s[3] - '0') * 10 + (s[4] - '0');
    int y = (s[6] - '0') * 1000 + (s[7] - '0') * 100 + (s[8] - '0') * 10 + (s[9] - '0');
    int h = (s[11] - '0') * 10 + (s[12] - '0');
    int mi = (s[14] - '0') * 10 + (s[15] - '0');
    int sec = (s[17] - '0') * 10 + (s[18] - '0');
    if (y < 1 || mo < 1 || mo > 12 || d < 1 || d > days_in_month(y, mo))
        return STATUS_ERR_INPUT;
    if (h > 23 || mi > 59 || sec > 59)
        return STATUS_ERR_INPUT;
    *key = make_time_key(y, mo, d, h, mi, sec);
    return STATUS_OK;
}

// current system time as a time key
status_t get_now_key(long long* key){
    if (key == NULL)
        return STATUS_ERR_NULL_PTR;
    time_t t = time(NULL);
    if (t == (time_t)-1)
        return STATUS_ERR_TIME;
    struct tm* lt = localtime(&t);
    if (lt == NULL)
        return STATUS_ERR_TIME;
    *key = make_time_key(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                         lt->tm_hour, lt->tm_min, lt->tm_sec);
    return STATUS_OK;
}

/* ---------- Post logic (no input / output here) ---------- */

// order of the mails: recipient index first, then identifier
int mail_compare(const Mail* a, const Mail* b){
    int c = string_compare_lex(&a->address.index, &b->address.index);
    if (c != 0)
        return c;
    return string_compare_lex(&a->id, &b->id);
}

// find a mail by identifier; position is stored to *pos
status_t post_find(const Post* p, const char* id, int* pos){
    if (p == NULL || id == NULL || pos == NULL)
        return STATUS_ERR_NULL_PTR;
    for (int i = 0; i < p->count; i++){
        if (strcmp(p->mails[i].id.data, id) == 0){
            *pos = i;
            return STATUS_OK;
        }
    }
    return STATUS_ERR_NOT_FOUND;
}

// put the mail into the sorted array; on success the post owns the mail
status_t post_add(Post* p, const Mail* m){
    if (p == NULL || m == NULL)
        return STATUS_ERR_NULL_PTR;
    int tmp;
    if (post_find(p, m->id.data, &tmp) == STATUS_OK)
        return STATUS_ERR_DUPLICATE;
    if (p->count == p->capacity){
        int new_cap = (p->capacity == 0) ? START_CAPACITY : p->capacity * 2;
        Mail* buf = (Mail*)realloc(p->mails, (size_t)new_cap * sizeof(Mail));
        if (buf == NULL)
            return STATUS_ERR_ALLOC;
        p->mails = buf;
        p->capacity = new_cap;
    }
    // find the first element which is greater, shift the tail, insert
    int pos = 0;
    while (pos < p->count && mail_compare(&p->mails[pos], m) < 0)
        pos++;
    memmove(&p->mails[pos + 1], &p->mails[pos], (size_t)(p->count - pos) * sizeof(Mail));
    p->mails[pos] = *m;
    p->count++;
    return STATUS_OK;
}

// remove the mail with the given identifier and free it
status_t post_remove(Post* p, const char* id){
    int pos;
    status_t st = post_find(p, id, &pos);
    if (st != STATUS_OK)
        return st;
    mail_destroy(&p->mails[pos]);
    memmove(&p->mails[pos], &p->mails[pos + 1], (size_t)(p->count - pos - 1) * sizeof(Mail));
    p->count--;
    return STATUS_OK;
}

status_t post_set_delivered(Post* p, const char* id){
    int pos;
    status_t st = post_find(p, id, &pos);
    if (st != STATUS_OK)
        return st;
    p->mails[pos].delivered = 1;
    return STATUS_OK;
}

// order by creation time (old first), ties by identifier
int compare_by_created(const void* pa, const void* pb){
    const Mail* a = *(const Mail* const*)pa;
    const Mail* b = *(const Mail* const*)pb;
    if (a->created_key != b->created_key)
        return (a->created_key < b->created_key) ? -1 : 1;
    return string_compare_lex(&a->id, &b->id);
}

// collect delivered mails, or mails not delivered whose deadline is before now;
// result is a malloc'ed array of pointers (may be NULL when *n == 0)
status_t post_collect(const Post* p, filter_t filter, long long now_key,
                      const Mail*** result, int* n){
    if (p == NULL || result == NULL || n == NULL)
        return STATUS_ERR_NULL_PTR;
    *result = NULL;
    *n = 0;
    if (p->count == 0)
        return STATUS_OK;
    const Mail** list = (const Mail**)malloc((size_t)p->count * sizeof(const Mail*));
    if (list == NULL)
        return STATUS_ERR_ALLOC;
    int cnt = 0;
    for (int i = 0; i < p->count; i++){
        const Mail* m = &p->mails[i];
        int take = (filter == FILTER_DELIVERED)
                       ? m->delivered
                       : (!m->delivered && m->deadline_key < now_key);
        if (take)
            list[cnt++] = m;
    }
    qsort(list, (size_t)cnt, sizeof(const Mail*), compare_by_created);
    *result = list;
    *n = cnt;
    return STATUS_OK;
}

/* ---------- output ---------- */

void print_status(status_t st){
    switch (st){
        case STATUS_OK:            break;
        case STATUS_ERR_NULL_PTR:  printf("Error: null pointer\n"); break;
        case STATUS_ERR_ALLOC:     printf("Error: failed to allocate memory\n"); break;
        case STATUS_ERR_INPUT:     printf("Error: wrong value or format\n"); break;
        case STATUS_ERR_NOT_FOUND: printf("Error: mail with such identifier not found\n"); break;
        case STATUS_ERR_DUPLICATE: printf("Error: mail with such identifier already exists\n"); break;
        case STATUS_ERR_EOF:       printf("Input ended\n"); break;
        case STATUS_ERR_TIME:      printf("Error: system time is not available\n"); break;
        default:                   printf("Other status\n"); break;
    }
}

void print_address(const Address* a){
    printf("%s, %s, house %d", a->index.data, a->city.data, a->house);
    printf(", street %s", a->street.data);
    if (a->building.length > 0)
        printf(", building %s", a->building.data);
    printf(", apartment %d", a->apartment);
}

void print_mail(const Mail* m){
    printf("Mail %s\n", m->id.data);
    printf("  recipient: ");
    print_address(&m->address);
    printf("\n  weight:    %.3f\n", m->weight);
    printf("  created:   %s\n", m->created.data);
    printf("  deadline:  %s\n", m->deadline.data);
    printf("  status:    %s\n", m->delivered ? "delivered" : "not delivered");
}

void print_mail_list(const Mail** list, int n){
    if (n == 0){
        printf("(nothing found)\n");
        return;
    }
    for (int i = 0; i < n; i++)
        print_mail(list[i]);
    printf("Total: %d\n", n);
}

void print_all_mails(const Post* p){
    if (p->count == 0){
        printf("(post is empty)\n");
        return;
    }
    for (int i = 0; i < p->count; i++)
        print_mail(&p->mails[i]);
    printf("Total: %d\n", p->count);
}

/* ---------- input (prompts + validation loops) ---------- */

// read a whole line of any length (without '\n' and '\r'); caller frees it
status_t read_line(char** line){
    if (line == NULL)
        return STATUS_ERR_NULL_PTR;
    *line = NULL;
    size_t cap = 32, len = 0;
    char* buf = (char*)malloc(cap);
    if (buf == NULL)
        return STATUS_ERR_ALLOC;
    int c;
    while ((c = getchar()) != EOF && c != '\n'){
        if (len + 1 >= cap){
            char* nb = (char*)realloc(buf, cap * 2);
            if (nb == NULL){
                free(buf);
                return STATUS_ERR_ALLOC;
            }
            buf = nb;
            cap *= 2;
        }
        buf[len++] = (char)c;
    }
    if (c == EOF && len == 0){
        free(buf);
        return STATUS_ERR_EOF;
    }
    buf[len] = '\0';
    if (len > 0 && buf[len - 1] == '\r')
        buf[len - 1] = '\0';
    *line = buf;
    return STATUS_OK;
}

// remove spaces at the beginning and at the end (in place)
void trim(char* s){
    size_t len = strlen(s), start = 0;
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        len--;
    while (start < len && isspace((unsigned char)s[start]))
        start++;
    memmove(s, s + start, len - start);
    s[len - start] = '\0';
}

// prompt and read one trimmed line
status_t ask_line(const char* prompt, char** line){
    printf("%s", prompt);
    fflush(stdout);
    status_t st = read_line(line);
    if (st == STATUS_OK)
        trim(*line);
    return st;
}

// string, which can be empty if allow_empty is set
status_t ask_string(const char* prompt, int allow_empty, String* out){
    for (;;){
        char* line = NULL;
        status_t st = ask_line(prompt, &line);
        if (st != STATUS_OK)
            return st;
        if (line[0] == '\0' && !allow_empty){
            printf("Error: the string must not be empty\n");
            free(line);
            continue;
        }
        st = string_create(out, line);
        free(line);
        return st;
    }
}

status_t ask_natural(const char* prompt, int* out){
    for (;;){
        char* line = NULL;
        status_t st = ask_line(prompt, &line);
        if (st != STATUS_OK)
            return st;
        st = parse_natural(line, out);
        free(line);
        if (st == STATUS_OK)
            return STATUS_OK;
        if (st != STATUS_ERR_INPUT)
            return st;
        printf("Error: a natural number is expected\n");
    }
}

status_t ask_weight(const char* prompt, double* out){
    for (;;){
        char* line = NULL;
        status_t st = ask_line(prompt, &line);
        if (st != STATUS_OK)
            return st;
        st = parse_weight(line, out);
        free(line);
        if (st == STATUS_OK)
            return STATUS_OK;
        if (st != STATUS_ERR_INPUT)
            return st;
        printf("Error: a non-negative real number is expected, for example 2.5\n");
    }
}

// string of exactly count digits
status_t ask_digits(const char* prompt, int count, String* out){
    for (;;){
        char* line = NULL;
        status_t st = ask_line(prompt, &line);
        if (st != STATUS_OK)
            return st;
        if (!is_digits(line, count)){
            printf("Error: exactly %d digits are expected\n", count);
            free(line);
            continue;
        }
        st = string_create(out, line);
        free(line);
        return st;
    }
}

// time in format dd:MM:yyyy hh:mm:ss
status_t ask_datetime(const char* prompt, String* out, long long* key){
    for (;;){
        char* line = NULL;
        status_t st = ask_line(prompt, &line);
        if (st != STATUS_OK)
            return st;
        st = parse_datetime(line, key);
        if (st == STATUS_OK){
            st = string_create(out, line);
            free(line);
            return st;
        }
        free(line);
        if (st != STATUS_ERR_INPUT)
            return st;
        printf("Error: expected a real date in format dd:MM:yyyy hh:mm:ss\n");
    }
}

status_t ask_address(Address* a){
    status_t st = ask_string("City: ", 0, &a->city);
    if (st == STATUS_OK) st = ask_string("Street: ", 0, &a->street);
    if (st == STATUS_OK) st = ask_natural("House number: ", &a->house);
    if (st == STATUS_OK) st = ask_string("Building (may be empty): ", 1, &a->building);
    if (st == STATUS_OK) st = ask_natural("Apartment number: ", &a->apartment);
    if (st == STATUS_OK) st = ask_digits("Postal index (6 digits): ", INDEX_LEN, &a->index);
    return st;
}

// identifier: 14 digits, not used by another mail
status_t ask_new_id(const Post* p, String* out){
    for (;;){
        status_t st = ask_digits("Mail identifier (14 digits): ", ID_LEN, out);
        if (st != STATUS_OK)
            return st;
        int pos;
        if (post_find(p, out->data, &pos) != STATUS_OK)
            return STATUS_OK;
        printf("Error: mail with such identifier already exists\n");
        string_destroy(out);
    }
}

// read all fields of a new mail; on error the partial mail must be destroyed
status_t ask_mail(const Post* p, Mail* m){
    mail_init(m);
    status_t st = ask_address(&m->address);
    if (st == STATUS_OK) st = ask_weight("Weight (kg): ", &m->weight);
    if (st == STATUS_OK) st = ask_new_id(p, &m->id);
    if (st == STATUS_OK) st = ask_datetime("Creation time (dd:MM:yyyy hh:mm:ss): ",
                                           &m->created, &m->created_key);
    while (st == STATUS_OK){
        st = ask_datetime("Delivery deadline (dd:MM:yyyy hh:mm:ss): ",
                          &m->deadline, &m->deadline_key);
        if (st != STATUS_OK || m->deadline_key >= m->created_key)
            break;
        printf("Error: the deadline must not be earlier than the creation time\n");
        string_destroy(&m->deadline);
    }
    return st;
}

#endif
