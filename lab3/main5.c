#define _XOPEN_SOURCE 700   // for realpath from posix
#include <sys/stat.h>
#include "functions5.h"

// same path (after realpath) or same inode (hard link) is the same file
status_t check_different(const char* in, const char* out){
    char real_in[PATH_MAX];
    char real_out[PATH_MAX];
    struct stat st_in, st_out;

    // trace file doesn't exist yet - nothing to compare
    if (realpath(out, real_out) == NULL)
        return STATUS_OK;
    if (realpath(in, real_in) != NULL && strcmp(real_in, real_out) == 0)
        return STATUS_ERR_SAME_FILE;
    if (stat(in, &st_in) == 0 && stat(out, &st_out) == 0 &&
        st_in.st_dev == st_out.st_dev && st_in.st_ino == st_out.st_ino)
        return STATUS_ERR_SAME_FILE;
    return STATUS_OK;
}

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

// small number: 1..9 digits
status_t parse_choice(const char* s, int* result){
    size_t len = strlen(s);
    if (len == 0 || len > 9)
        return STATUS_ERR_INPUT;
    int v = 0;
    for (size_t i = 0; i < len; i++){
        if (!isdigit((unsigned char)s[i]))
            return STATUS_ERR_INPUT;
        v = v * 10 + (s[i] - '0');
    }
    *result = v;
    return STATUS_OK;
}

void print_menu(void){
    printf("\n[students]\n"
           "find by:\n1 - id\n2 - surname\n3 - name\n4 - group\n\n"
           "5 - sort\n\n6 - students with average > average of all\n\n0 - exit\n");
}

// choices 1-4: ask a key, write students to the trace file
status_t action_find(const Student* arr, size_t n, field_t field, FILE* trace){
    static const char* const prompt[] = {"Id: ", "Surname: ", "Name: ", "Group: "};
    static const char* const title[] = {"id", "surname", "name", "group"};
    char key[LINE_SIZE];
    unsigned int id = 0;
    size_t* idx = NULL;
    size_t cnt = 0;

    status_t st = read_line(prompt[field], key, LINE_SIZE);
    if (st == STATUS_OK){
        if (field == FIELD_ID)
            st = (parse_id(key, &id) == STATUS_OK) ? STATUS_OK : STATUS_ERR_INPUT;
        else if (field == FIELD_GROUP)
            st = is_group(key) ? STATUS_OK : STATUS_ERR_INPUT;
        else
            st = is_latin_word(key) ? STATUS_OK : STATUS_ERR_INPUT;
    }
    if (st == STATUS_OK)
        st = find_students(arr, n, field, key, id, &idx, &cnt);
    if (st == STATUS_OK && fprintf(trace, "Search by %s \"%s\": %zu found\n", title[field], key, cnt) < 0)
        st = STATUS_ERR_WRITE;
    if (st == STATUS_OK)
        st = write_students(trace, arr, idx, cnt);
    if (st == STATUS_OK)
        printf("Found: %zu (written to the trace file)\n", cnt);
    free(idx);
    return st;
}

// choice 5: sort the array, write the sorted list to the trace file
status_t action_sort(Student* arr, size_t n, FILE* trace){
    static const char* const title[] = {"id", "surname", "name", "group"};
    comparator_t cmp[] = {compare_by_id, compare_by_surname, compare_by_name, compare_by_group};
    char line[LINE_SIZE];
    int choice = 0;

    printf("Sort by: 1 - id, 2 - surname, 3 - name, 4 - group\n");
    status_t st = read_line("Your choice: ", line, LINE_SIZE);
    if (st == STATUS_OK && (parse_choice(line, &choice) != STATUS_OK || choice < 1 || choice > 4))
        st = STATUS_ERR_INPUT;
    if (st == STATUS_OK)
        st = sort_students(arr, n, cmp[choice - 1]);
    if (st == STATUS_OK && fprintf(trace, "Sorted by %s:\n", title[choice - 1]) < 0)
        st = STATUS_ERR_WRITE;
    if (st == STATUS_OK)
        st = write_students(trace, arr, NULL, n);
    if (st == STATUS_OK)
        printf("Sorted (written to the trace file)\n");
    return st;
}

// choice 6: surnames and names of students with a better average than all students have
status_t action_above(const Student* arr, size_t n, FILE* trace){
    size_t* idx = NULL;
    size_t cnt = 0;
    status_t st = find_above_average(arr, n, &idx, &cnt);
    if (st == STATUS_OK && fprintf(trace, "Average higher than the average of all students: %zu\n", cnt) < 0)
        st = STATUS_ERR_WRITE;
    if (st == STATUS_OK)
        st = write_names(trace, arr, idx, cnt);
    if (st == STATUS_OK)
        printf("Found: %zu (written to the trace file)\n", cnt);
    free(idx);
    return st;
}



































status_t run_choice(Student* arr, size_t n, FILE* trace, int choice){
    switch (choice){
        case 1: return action_find(arr, n, FIELD_ID, trace);
        case 2: return action_find(arr, n, FIELD_SURNAME, trace);
        case 3: return action_find(arr, n, FIELD_NAME, trace);
        case 4: return action_find(arr, n, FIELD_GROUP, trace);
        case 5: return action_sort(arr, n, trace);
        case 6: return action_above(arr, n, trace);
        default: printf("Unknown menu item\n"); return STATUS_OK;
    }
}

// read the students file, then open the trace file
status_t open_files(const char* path_in, const char* path_trace, Student** arr, size_t* n,
                    FILE** trace, size_t* bad_line){
    status_t st = check_different(path_in, path_trace);
    if (st != STATUS_OK)
        return st;

    FILE* in = fopen(path_in, "r");
    if (in == NULL)
        return STATUS_ERR_OPEN_IN;
    st = read_students(in, arr, n, bad_line);
    fclose(in);
    if (st != STATUS_OK)
        return st;

    // the trace file is created only when the input is correct
    *trace = fopen(path_trace, "w");
    if (*trace == NULL){
        students_destroy(*arr, *n);
        *arr = NULL;
        return STATUS_ERR_OPEN_OUT;
    }
    return STATUS_OK;
}

int main(int argc, char* argv[]){
    if (argc != 3){
        printf("Usage:\n  %s <students file> <trace file>\n", argv[0]);
        return 1;
    }

    Student* arr = NULL;
    size_t n = 0, bad_line = 0;
    FILE* trace = NULL;
    status_t st = open_files(argv[1], argv[2], &arr, &n, &trace, &bad_line);
    if (st != STATUS_OK){
        print_status(st, bad_line);
        return 1;
    }
    printf("Loaded students: %zu\n", n);

    while (st == STATUS_OK){
        char line[LINE_SIZE];
        int choice = -1;
        print_menu();
        st = read_line("Your choice: ", line, LINE_SIZE);
        if (st != STATUS_OK && st != STATUS_ERR_INPUT)
            break;
        if (st == STATUS_OK && strcmp(line, "0") == 0)
            break;
        if (st == STATUS_OK && parse_choice(line, &choice) != STATUS_OK)
            choice = -1;
        st = run_choice(arr, n, trace, choice);
        // an error of one action is only reported, the dialog goes on
        if (st != STATUS_OK && st != STATUS_ERR_EOF && st != STATUS_ERR_ALLOC){
            print_status(st, 0);
            st = STATUS_OK;
        }
    }
    if (st != STATUS_OK)
        print_status(st, 0);

    if (fclose(trace) != 0)
        printf("Error: cannot write trace file\n");
    students_destroy(arr, n);
    return st == STATUS_OK ? 0 : 1;
}
