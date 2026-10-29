#include <stdio.h>
#include "functions5.h"


int main(void){
    char buf[512];
    int r;

    // 1. roman numerals
    printf("1) %%Ro: 1994, 4, 3999\n");
    r = oversprintf(buf, "%Ro %Ro %Ro", 1994, 4, 3999);
    if (r >= 0) printf("Result: %s (%d chars)\n", buf, r);
    print_status(r);

    // 2. roman: error (0 is not allowed)
    printf("\n2) %%Ro: 0\n");
    r = oversprintf(buf, "%Ro", 0);
    print_status(r);

    // 3. zeckendorf
    printf("\n3) %%Zr: 17, 1, 2, 3\n");
    r = oversprintf(buf, "%Zr %Zr %Zr %Zr", 17u, 1u, 2u, 3u);
    if (r >= 0) printf("Result: %s\n", buf);
    print_status(r);

    // 4. zeckendorf: error (0 has no representation)
    printf("\n4) %%Zr: 0\n");
    r = oversprintf(buf, "%Zr", 0u);
    print_status(r);

    // 5. number -> base
    printf("\n5) %%Cv / %%CV: 255 (16) lower and upper, -255 (2), 35 (36), 255 (99 -> base 10)\n");
    r = oversprintf(buf, "%Cv %CV %Cv %Cv %Cv", 255, 16, 255, 16, -255, 2, 35, 36, 255, 99);
    if (r >= 0) printf("Result: %s\n", buf);
    print_status(r);

    // 6. base -> number
    printf("\n6) %%to / %%TO: \"ff\" (16), \"FF\" (16), \"-101\" (2), \"zz\" (36)\n");
    r = oversprintf(buf, "%to %TO %to %to", "ff", 16, "FF", 16, "-101", 2, "zz", 36);
    if (r >= 0) printf("Result: %s\n", buf);
    print_status(r);

    // 7. error: upper case letters in %to
    printf("\n7) %%to: \"FF\" (16)\n");
    r = oversprintf(buf, "%to", "FF", 16);
    print_status(r);

    // 8. error: digit >= base
    printf("\n8) %%to: \"102\" (2)\n");
    r = oversprintf(buf, "%to", "102", 2);
    print_status(r);

    // 9. memory dump
    printf("\n9) %%mi / %%mu / %%mf / %%md\n");
    r = oversprintf(buf, "%mi\n%mu\n%mf\n%md", 5, 4294967295u, 1.0f, 1.0);
    if (r >= 0) printf("%s\n", buf);
    print_status(r);

    // 10. our and standard specifiers together
    printf("\n10) mixed format\n");
    r = oversprintf(buf, "[%5d|%-5d|%05.1f|%s|%c|%x|%ld|%*d|%.*f|%%] year %Ro",
                    42, 42, 3.14159, "text", 'Z', 255, 123456789L, 6, 7, 2, 2.71828, 2024);
    if (r >= 0) printf("Result: %s\n", buf);
    print_status(r);

    // 11. %n
    printf("\n11) %%n\n");
    int cnt = 0;
    r = oversprintf(buf, "abc%nde", &cnt);
    if (r >= 0) printf("Result: %s, %%n = %d\n", buf, cnt);
    print_status(r);

    // 12. overfprintf into a file
    printf("\n12) overfprintf to stdout\n");
    r = overfprintf(stdout, "Hello %Ro, %Cv, %to\n", 14, 255, 16, "1a", 16);
    print_status(r);

    // 13. error: unknown specifier
    printf("\n13) %%q\n");
    r = oversprintf(buf, "%q", 1);
    print_status(r);

    // 14. error: NULL pointers
    printf("\n14) NULL buffer\n");
    r = oversprintf(NULL, "x");
    print_status(r);

    return 0;
}
