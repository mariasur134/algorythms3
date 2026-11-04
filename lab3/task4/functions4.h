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
#define LINE_SIZE 256

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
    s->data = NULL;
    s->length = 0;
    if (len >= (size_t)INT_MAX)
        return STATUS_ERR_INPUT;
    s->data = (char*)malloc(len + 1);
    if (s->data == NULL)
        return STATUS_ERR_ALLOC;
    memcpy(s->data, src, len + 1);
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

// lexicographic comparator: <0, 0, >0
int string_compare_lex(const String* a, const String* b){
    return strcmp(a->data, b->data);
}

// equivalence relation: 1 if the strings are equal
int string_equal(const String* a, const String* b){
    return string_compare_lex(a, b) == 0;
}

// order relation: by length first, then lexicographically: <0, 0, >0
int string_compare_order(const String* a, const String* b){
    if (a->length != b->length)
        return (a->length < b->length) ? -1 : 1;
    return string_compare_lex(a, b);
}

// copy src into an existing String; old content is replaced only on success
status_t string_copy(String* dst, const String* src){
    if (dst == NULL || src == NULL)
        return STATUS_ERR_NULL_PTR;
    String tmp;
    status_t st = string_create(&tmp, src->data);
    if (st != STATUS_OK)
        return st;
    string_destroy(dst);
    *dst = tmp;
    return STATUS_OK;
}

// copy src into a new String placed in the heap; free it with string_free
status_t string_clone(const String* src, String** result){
    if (src == NULL || result == NULL)
        return STATUS_ERR_NULL_PTR;
    *result = (String*)malloc(sizeof(String));
    if (*result == NULL)
        return STATUS_ERR_ALLOC;
    status_t st = string_create(*result, src->data);
    if (st != STATUS_OK){
        free(*result);
        *result = NULL;
    }
    return st;
}

// free a String created by string_clone (content and the struct itself)
void string_free(String* s){
    string_destroy(s);
    free(s);
}

// append src to dst; src may be the same object as dst
status_t string_concat(String* dst, const String* src){
    if (dst == NULL || src == NULL)
        return STATUS_ERR_NULL_PTR;
    size_t len_dst = (size_t)dst->length, len_src = (size_t)src->length;
    if (len_dst + len_src >= (size_t)INT_MAX)
        return STATUS_ERR_INPUT;
    char* buf = (char*)malloc(len_dst + len_src + 1);
    if (buf == NULL)
        return STATUS_ERR_ALLOC;
    memcpy(buf, dst->data, len_dst);
    memcpy(buf + len_dst, src->data, len_src + 1);
    free(dst->data);
    dst->data = buf;
    dst->length = (int)(len_dst + len_src);
    return STATUS_OK;
}

/* ======================= Part 2: Address, Mail, Post ======================= */

typedef struct {
    String city, street;    // not empty
    int house;              // natural number
    String building;        // may be empty
    int apartment;          // natural number
    String index;           // 6 digits
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
    Address* office;        // address of the current post office
    Mail* mails;            // dynamic array, always sorted by (index, id)
    int count;
    int capacity;
} Post;

void address_destroy(Address* a){
    string_destroy(&a->city);
    string_destroy(&a->street);
    string_destroy(&a->building);
    string_destroy(&a->index);
}

void mail_destroy(Mail* m){
    address_destroy(&m->address);
    string_destroy(&m->id);
    string_destroy(&m->created);
    string_destroy(&m->deadline);
}

void post_destroy(Post* p){
    for (int i = 0; i < p->count; i++)
        mail_destroy(&p->mails[i]);
    free(p->mails);
    if (p->office != NULL)
        address_destroy(p->office);
    free(p->office);
}

/* ---------- validation ---------- */

// 1 if s consists of exactly count digits
int is_digits(const char* s, int count){
    if ((int)strlen(s) != count)
        return 0;
    for (int i = 0; i < count; i++)
        if (!isdigit((unsigned char)s[i]))
            return 0;
    return 1;
}

// natural number: only digits, value in [1, INT_MAX]
status_t parse_natural(const char* s, int* result){
    long long v = 0;
    if (*s == '\0')
        return STATUS_ERR_INPUT;
    for (; *s != '\0'; s++){
        if (!isdigit((unsigned char)*s))
            return STATUS_ERR_INPUT;
        v = v * 10 + (*s - '0');
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
    *result = strtod(s, NULL);
    return isfinite(*result) ? STATUS_OK : STATUS_ERR_INPUT;
}

int days_in_month(int y, int m){
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    return (m == 2 && leap) ? 29 : days[m - 1];
}

// number yyyyMMddhhmmss, which keeps the chronological order
long long make_time_key(int y, int mo, int d, int h, int mi, int s){
    return ((((((long long)y * 100 + mo) * 100 + d) * 100 + h) * 100 + mi) * 100) + s;
}

// check "dd:MM:yyyy hh:mm:ss" (format, ranges, leap years) and compute its key
status_t parse_datetime(const char* s, long long* key){
    int d, mo, y, h, mi, sec;
    // sscanf is tolerant to spaces and signs, so check the exact shape first
    if ((int)strlen(s) != DATETIME_LEN)
        return STATUS_ERR_INPUT;
    for (int i = 0; i < DATETIME_LEN; i++){
        char sep = (i == 10) ? ' ' : ':';
        int is_sep = (i == 2 || i == 5 || i == 10 || i == 13 || i == 16);
        if (is_sep ? s[i] != sep : !isdigit((unsigned char)s[i]))
            return STATUS_ERR_INPUT;
    }
    if (sscanf(s, "%d:%d:%d %d:%d:%d", &d, &mo, &y, &h, &mi, &sec) != 6)
        return STATUS_ERR_INPUT;
    if (y < 1 || mo < 1 || mo > 12 || d < 1 || d > days_in_month(y, mo)
        || h > 23 || mi > 59 || sec > 59)
        return STATUS_ERR_INPUT;
    *key = make_time_key(y, mo, d, h, mi, sec);
    return STATUS_OK;
}

// current system time as a time key
status_t get_now_key(long long* key){
    time_t t = time(NULL);
    struct tm* lt = localtime(&t);
    if (t == (time_t)-1 || lt == NULL)
        return STATUS_ERR_TIME;
    *key = make_time_key(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                         lt->tm_hour, lt->tm_min, lt->tm_sec);
    return STATUS_OK;
}

/* ---------- Post logic ---------- */

// order of the mails: recipient index first, then identifier
int mail_compare(const Mail* a, const Mail* b){
    int c = string_compare_lex(&a->address.index, &b->address.index);
    return c != 0 ? c : string_compare_lex(&a->id, &b->id);
}

// find a mail by identifier; position is stored to *pos
status_t post_find(const Post* p, const char* id, int* pos){
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
    int pos;
    if (post_find(p, m->id.data, &pos) == STATUS_OK)
        return STATUS_ERR_DUPLICATE;
    if (p->count == p->capacity){
        int new_cap = (p->capacity == 0) ? 4 : p->capacity * 2;
        Mail* buf = (Mail*)realloc(p->mails, (size_t)new_cap * sizeof(Mail));
        if (buf == NULL)
            return STATUS_ERR_ALLOC;
        p->mails = buf;
        p->capacity = new_cap;
    }
    // first element which is greater than the new one, shift the tail, insert
    for (pos = 0; pos < p->count && mail_compare(&p->mails[pos], m) < 0; pos++)
        ;
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
    if (st == STATUS_OK)
        p->mails[pos].delivered = 1;
    return st;
}

// order by creation time (old first), ties by identifier
int compare_by_created(const void* pa, const void* pb){
    const Mail* a = *(const Mail* const*)pa;
    const Mail* b = *(const Mail* const*)pb;
    if (a->created_key != b->created_key)
        return (a->created_key < b->created_key) ? -1 : 1;
    return string_compare_lex(&a->id, &b->id);
}

// collect delivered mails, or not delivered ones whose deadline is before now;
// result is a malloc'ed array of pointers sorted by creation time
status_t post_collect(const Post* p, filter_t filter, long long now,
                      const Mail*** result, int* n){
    *result = NULL;
    *n = 0;
    if (p->count == 0)
        return STATUS_OK;
    const Mail** list = (const Mail**)malloc((size_t)p->count * sizeof(const Mail*));
    if (list == NULL)
        return STATUS_ERR_ALLOC;
    for (int i = 0; i < p->count; i++){
        const Mail* m = &p->mails[i];
        if (filter == FILTER_DELIVERED ? m->delivered
                                       : (!m->delivered && m->deadline_key < now))
            list[(*n)++] = m;
    }
    qsort(list, (size_t)*n, sizeof(const Mail*), compare_by_created);
    *result = list;
    return STATUS_OK;
}

/* ---------- output ---------- */

void print_status(status_t st){
    static const char* const text[] = {
        "ok", "null pointer", "failed to allocate memory", "wrong value or format",
        "mail with such identifier not found", "mail with such identifier already exists",
        "input ended", "system time is not available"
    };
    if (st != STATUS_OK)
        printf("Error: %s\n", text[st]);
}

void print_address(const Address* a){
    printf("%s, %s, street %s, house %d", a->index.data, a->city.data, a->street.data, a->house);
    if (a->building.length > 0)
        printf(", building %s", a->building.data);
    printf(", apartment %d", a->apartment);
}

void print_mail(const Mail* m){
    printf("Mail %s\n  recipient: ", m->id.data);
    print_address(&m->address);
    printf("\n  weight:    %.3f\n  created:   %s\n  deadline:  %s\n  status:    %s\n",
           m->weight, m->created.data, m->deadline.data,
           m->delivered ? "delivered" : "not delivered");
}

void print_mails(const Mail* mails, const Mail** list, int n){
    for (int i = 0; i < n; i++)
        print_mail(list != NULL ? list[i] : &mails[i]);
    printf(n == 0 ? "(nothing found)\n" : "Total: %d\n", n);
}

/* ---------- input: one prompt, one line, one validated value ---------- */

// prompt and read one line (without '\n') into buf
status_t read_line(const char* prompt, char* buf, int size){
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, size, stdin) == NULL)
        return STATUS_ERR_EOF;
    size_t len = strcspn(buf, "\r\n");
    if (buf[len] == '\0'){                      // line did not fit into buf
        int c;
        while ((c = getchar()) != EOF && c != '\n')
            ;
        return STATUS_ERR_INPUT;
    }
    buf[len] = '\0';
    return STATUS_OK;
}

status_t read_string(const char* prompt, int allow_empty, String* out){
    char buf[LINE_SIZE];
    status_t st = read_line(prompt, buf, LINE_SIZE);
    if (st == STATUS_OK && buf[0] == '\0' && !allow_empty)
        st = STATUS_ERR_INPUT;
    return st == STATUS_OK ? string_create(out, buf) : st;
}

status_t read_natural(const char* prompt, int* out){
    char buf[LINE_SIZE];
    status_t st = read_line(prompt, buf, LINE_SIZE);
    return st == STATUS_OK ? parse_natural(buf, out) : st;
}

// string of exactly count digits
status_t read_digits(const char* prompt, int count, String* out){
    char buf[LINE_SIZE];
    status_t st = read_line(prompt, buf, LINE_SIZE);
    if (st == STATUS_OK && !is_digits(buf, count))
        st = STATUS_ERR_INPUT;
    return st == STATUS_OK ? string_create(out, buf) : st;
}

status_t read_datetime(const char* prompt, String* out, long long* key){
    char buf[LINE_SIZE];
    status_t st = read_line(prompt, buf, LINE_SIZE);
    if (st == STATUS_OK)
        st = parse_datetime(buf, key);
    return st == STATUS_OK ? string_create(out, buf) : st;
}

// fields of a failed read stay NULL, so address_destroy is always safe
void address_init(Address* a){
    memset(a, 0, sizeof(Address));
}

status_t read_address(Address* a){
    address_init(a);
    status_t st = read_string("City: ", 0, &a->city);
    if (st == STATUS_OK) st = read_string("Street: ", 0, &a->street);
    if (st == STATUS_OK) st = read_natural("House number: ", &a->house);
    if (st == STATUS_OK) st = read_string("Building (may be empty): ", 1, &a->building);
    if (st == STATUS_OK) st = read_natural("Apartment number: ", &a->apartment);
    if (st == STATUS_OK) st = read_digits("Postal index (6 digits): ", INDEX_LEN, &a->index);
    return st;
}

// read all fields of a new mail; on error the partial mail must be destroyed
status_t read_mail(Mail* m){
    char buf[LINE_SIZE];
    memset(m, 0, sizeof(Mail));
    status_t st = read_address(&m->address);
    if (st == STATUS_OK) st = read_line("Weight (kg): ", buf, LINE_SIZE);
    if (st == STATUS_OK) st = parse_weight(buf, &m->weight);
    if (st == STATUS_OK) st = read_digits("Mail identifier (14 digits): ", ID_LEN, &m->id);
    if (st == STATUS_OK) st = read_datetime("Creation time (dd:MM:yyyy hh:mm:ss): ",
                                            &m->created, &m->created_key);
    if (st == STATUS_OK) st = read_datetime("Delivery deadline (dd:MM:yyyy hh:mm:ss): ",
                                            &m->deadline, &m->deadline_key);
    if (st == STATUS_OK && m->deadline_key < m->created_key)
        st = STATUS_ERR_INPUT;      // deadline earlier than creation
    return st;
}

#endif
