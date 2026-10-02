#ifndef FUNCTIONS4_H
#define FUNCTIONS4_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL_PTR,        // NULL
    STATUS_ERR_ARGC,            // invalid number of arguments
    STATUS_ERR_FLAG_UNKNOWN,    // invalid argument
    STATUS_ERR_ALLOC,           // allocation error
    STATUS_ERR_OPEN_INPUT,      // couldn't open file
    STATUS_ERR_OPEN_OUTPUT      // couldnt' open file
} status_t;


// parsing flag
// -d or /d -> flag='d', has_n=0; -nd or /nd -> flag='d', has_n=1
status_t parse_flag(const char *str, char *out_flag, int *out_has_n)
{
    if (str == NULL || out_flag == NULL || out_has_n == NULL)
        return STATUS_ERR_NULL_PTR;

    const size_t len = strlen(str);
    if (len < 2 || (str[0] != '-' && str[0] != '/'))
        return STATUS_ERR_FLAG_UNKNOWN;

    char c;
    int has_n;
    if (len == 2) {
        c = str[1];
        has_n = 0;
    } else if (len == 3 && str[1] == 'n') {
        c = str[2];
        has_n = 1;
    } else {
        return STATUS_ERR_FLAG_UNKNOWN;
    }

    if (c != 'd' && c != 'i' && c != 's' && c != 'a')
        return STATUS_ERR_FLAG_UNKNOWN;

    *out_flag = c;
    *out_has_n = has_n;
    return STATUS_OK;
}

// creating output file name
status_t build_output_path(const char *input_path, char **out_path)
{
    if (input_path == NULL || out_path == NULL)
        return STATUS_ERR_NULL_PTR;

    const size_t len = strlen(input_path);
    char *path = (char *)malloc(len + 4 + 1);   // "out_" + input_path + '\0'
    if (path == NULL)
        return STATUS_ERR_ALLOC;

    strcpy(path, "out_");
    strcat(path, input_path);

    *out_path = path;
    return STATUS_OK;
}


status_t open_files(const char *input_path, const char *output_path, FILE **in, FILE **out)
{
    if (input_path == NULL || output_path == NULL || in == NULL || out == NULL)
        return STATUS_ERR_NULL_PTR;

    FILE *input_file = fopen(input_path, "r");
    if (input_file == NULL)
        return STATUS_ERR_OPEN_INPUT;

    FILE *output_file = fopen(output_path, "w");
    if (output_file == NULL) {
        fclose(input_file);
        return STATUS_ERR_OPEN_OUTPUT;
    }

    *in = input_file;
    *out = output_file;
    return STATUS_OK;
}

// -d:  remove arab digits
status_t flag_d(const char *input_path, const char *output_path)
{
    FILE *in = NULL, *out = NULL;
    const status_t st = open_files(input_path, output_path, &in, &out);
    if (st != STATUS_OK)
        return st;

    int ch;
    while ((ch = fgetc(in)) != EOF) {
        if (ch >= '0' && ch <= '9')
            continue;
        fputc(ch, out);
    }

    fclose(in);
    fclose(out);
    return STATUS_OK;
}

// -a: replace with ascii code
status_t flag_a(const char *input_path, const char *output_path)
{
    FILE *in = NULL, *out = NULL;
    const status_t st = open_files(input_path, output_path, &in, &out);
    if (st != STATUS_OK)
        return st;

    int ch;
    while ((ch = fgetc(in)) != EOF) {
        if ((ch >= '0' && ch <= '9') || ch == ' ' || ch == '\n') {
            fputc(ch, out);
            continue;
        }
        fprintf(out, "%X", (unsigned int)(unsigned char)ch);
    }

    fclose(in);
    fclose(out);
    return STATUS_OK;
}

// -i: count latin letters
status_t flag_i(const char *input_path, const char *output_path)
{
    FILE *in = NULL, *out = NULL;
    const status_t st = open_files(input_path, output_path, &in, &out);
    if (st != STATUS_OK)
        return st;

    unsigned long line_no = 1, count = 0;
    int has_pending = 0;             // without '\n'
    int ch;
    while ((ch = fgetc(in)) != EOF) {
        has_pending = 1;
        if (ch == '\n') {
            fprintf(out, "%lu: %lu\n", line_no, count);
            line_no++;
            count = 0;
            has_pending = 0;
            continue;
        }
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
            count++;
    }
    if (has_pending)
        fprintf(out, "%lu: %lu\n", line_no, count);

    fclose(in);
    fclose(out);
    return STATUS_OK;
}

// -s: another count
status_t flag_s(const char *input_path, const char *output_path)
{
    FILE *in = NULL, *out = NULL;
    const status_t st = open_files(input_path, output_path, &in, &out);
    if (st != STATUS_OK)
        return st;

    unsigned long line_no = 1, count = 0;
    int has_pending = 0;
    int ch;
    while ((ch = fgetc(in)) != EOF) {
        has_pending = 1;
        if (ch == '\n') {
            fprintf(out, "%lu: %lu\n", line_no, count);
            line_no++;
            count = 0;
            has_pending = 0;
            continue;
        }
        const int is_letter = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
        const int is_digit  = (ch >= '0' && ch <= '9');
        if (!is_letter && !is_digit && ch != ' ')
            count++;
    }
    if (has_pending)
        fprintf(out, "%lu: %lu\n", line_no, count);

    fclose(in);
    fclose(out);
    return STATUS_OK;
}

#endif