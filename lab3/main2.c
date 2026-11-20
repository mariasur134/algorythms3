#include "functions2.h"

int main(void){
    Talk* talks = NULL;
    int n = 0, k = 0, answer = 0;

    fprintf(stderr, "usage: n - essays, k - classes\nfirst line \"n k\", then n lines \"id HH:MM HH:MM\"\n");
    status_t st = read_input(stdin, &talks, &n, &k);
    if (st == STATUS_OK)
        st = max_talks(talks, n, k, &answer);
    if (st != STATUS_OK){
        fprintf(stderr, "Error: %s\n", status_message(st));
        free(talks);
        return 1;
    }
    printf("%d\n", answer);
    free(talks);
    return 0;
}

