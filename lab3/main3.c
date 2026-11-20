#include "functions3.h"

int main(int argc, char** argv){
    if (argc != 2){
        printf("usage: %s <file>\n", argv[0]);
        return 1;
    }
    Analysis a;
    status_t st = analyze_file(argv[1], &a);
    if (st != STATUS_OK){
        printf("error: %s\n", status_message(st));
        return 1;
    }
    analysis_print(stdout, &a);
    analysis_destroy(&a);
    return 0;
}
