#include "functions3.h"

int main(void){
    FileRes* r = NULL;
    status_t s;

    // 1. simple substring in two files
    printf("1) \"ab\" in t3_in1.txt, t3_in2.txt\n");
    s = find_all(&r, "ab", 2, "t3_in1.txt", "t3_in2.txt");
    print_res(s, r, 2);
    if (s == STATUS_OK) free_res(r, 2);
    r = NULL;

    // 2. substring with '\n'
    printf("\n2) \"b\\nx\" in t3_in1.txt\n");
    s = find_all(&r, "b\nx", 1, "t3_in1.txt");
    print_res(s, r, 1);
    if (s == STATUS_OK) free_res(r, 1);
    r = NULL;

    // 3. overlapping + empty file
    printf("\n3) \"aa\" in t3_in2.txt, t3_in3.txt (t3_in3.txt is empty)\n");
    s = find_all(&r, "aa", 2, "t3_in2.txt", "t3_in3.txt");
    print_res(s, r, 2);
    if (s == STATUS_OK) free_res(r, 2);
    r = NULL;

    // 4. error: empty substring
    printf("\n4) \"\" in t3_in1.txt\n");
    s = find_all(&r, "", 1, "t3_in1.txt");
    print_res(s, r, 1);
    if (s == STATUS_OK) free_res(r, 1);
    r = NULL;

    // 5. error: file does not exist
    printf("\n5) \"ab\" in t3_in1.txt, somebs.txt\n");
    s = find_all(&r, "ab", 2, "t3_in1.txt", "somebs.txt");
    print_res(s, r, 2);
    if (s == STATUS_OK) free_res(r, 2);
    r = NULL;

    return 0;
}
