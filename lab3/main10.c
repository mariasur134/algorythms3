#define _XOPEN_SOURCE 700   // for realpath
#include <limits.h>
#include <sys/stat.h>
#include "functions10.h"

// same path (after realpath) or same inode (hard link) is the same file
status_t check_different(const char* in, const char* out){
    char real_in[PATH_MAX];
    char real_out[PATH_MAX];
    struct stat st_in, st_out;

    // output file doesn't exist yet - nothing to compare
    if (realpath(out, real_out) == NULL)
        return STATUS_OK;
    if (realpath(in, real_in) != NULL && strcmp(real_in, real_out) == 0)
        return STATUS_ERR_SAME_FILE;
    if (stat(in, &st_in) == 0 && stat(out, &st_out) == 0 &&
        st_in.st_dev == st_out.st_dev && st_in.st_ino == st_out.st_ino)
        return STATUS_ERR_SAME_FILE;
    return STATUS_OK;
}

int main(int argc, char** argv){
    if (argc != 3){
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }
    status_t st = check_different(argv[1], argv[2]);
    if (st != STATUS_OK){
        printf("Error: %s\n", status_message(st));
        return 1;
    }
    FILE* in = fopen(argv[1], "r");
    if (in == NULL){
        printf("Error: %s\n", status_message(STATUS_ERR_OPEN_IN));
        return 1;
    }
    FILE* out = fopen(argv[2], "w");
    if (out == NULL){
        fclose(in);
        printf("Error: %s\n", status_message(STATUS_ERR_OPEN_OUT));
        return 1;
    }

    size_t bad = 0;
    st = process_stream(in, out, &bad);
    fclose(in);
    if (fclose(out) != 0 && st == STATUS_OK)
        st = STATUS_ERR_WRITE;
    if (st != STATUS_OK){
        printf("Error: %s\n", status_message(st));
        return 1;
    }
    if (bad > 0)
        printf("Done, but %zu line(s) were wrong, see the output file\n", bad);
    else
        printf("Done, trees are in %s\n", argv[2]);
    return 0;
}
