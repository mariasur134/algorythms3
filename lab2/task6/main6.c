#include <stdio.h>
#include "functions6.h"

int main(void) {
    int r, a = 0, b = 0;
    unsigned int u = 0;

    printf("1) custom spec, valid input\n");
    r = oversscanf("XIV 1010011 ff", "%Ro %Zr %Cv", &a, &u, &b, 16);
    print_scan_result(r);
    printf("Values: %d %u %d\n", a, u, b);         // 14 17 255

    printf("\n2) mixed\n");
    double x = 0;
    r = oversscanf("year=XIV 3.5", "year=%Ro %lf", &a, &x);
    print_scan_result(r);
    printf("Values: %d %.1f\n", a, x);

    printf("\n3) 1/2 invalid data\n");
    print_scan_result(oversscanf("XIV abc", "%Ro %Ro", &a, &b));

    printf("\n4) base = 99 -> base = 10\n");
    print_scan_result(oversscanf("1", "%Cv", &a, 99));

    printf("\n5) errors: null, unknown spec\n");
    print_scan_result(oversscanf(NULL, "%d", &a));
    print_scan_result(oversscanf("1", "%q", &a));

    printf("\n6) empty\n");
    print_scan_result(oversscanf("", "%Ro", &a));

    printf("\n7) file\n");
    FILE* f = fopen("test6.txt", "r");
    if (f == NULL){
        printf("Error: cannot open input.txt\n");
        return 1;
    }
    int c = 0;
    r = overfscanf(f, "%d %Ro %Cv", &a, &b, &c, 16);
    fclose(f);
    print_scan_result(r);
    printf("Values: %d %d %d\n", a, b, c);      // 7 42 -31
}
