#include <stdio.h>
#include "functions6.h"


int main(void){
    int r;

    // 1. roman numerals
    printf("1) %%Ro: \"MCMXCIV IV MMMCMXCIX\"\n");
    int a = 0, b = 0, c = 0;
    r = oversscanf("MCMXCIV IV MMMCMXCIX", "%Ro %Ro %Ro", &a, &b, &c);
    print_scan_result(r);
    printf("Values: %d %d %d\n", a, b, c);

    // 2. zeckendorf
    printf("\n2) %%Zr: \"1010011 11 011\"\n");
    unsigned int u1 = 0, u2 = 0, u3 = 0;
    r = oversscanf("1010011 11 011", "%Zr %Zr %Zr", &u1, &u2, &u3);
    print_scan_result(r);
    printf("Values: %u %u %u\n", u1, u2, u3);

    // 3. base -> number
    printf("\n3) %%Cv / %%CV: \"ff FF -101 zz 255\"\n");
    int x1 = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0;
    r = oversscanf("ff FF -101 zz 255", "%Cv %CV %Cv %Cv %Cv", &x1, 16, &x2, 16, &x3, 2, &x4, 36, &x5, 99);
    print_scan_result(r);
    printf("Values: %d %d %d %d %d\n", x1, x2, x3, x4, x5);

    // 4. wrong case of letters: %Cv does not take "FF"
    printf("\n4) %%Cv: \"FF\" (16)\n");
    int y = 0;
    r = oversscanf("FF", "%Cv", &y, 16);
    print_scan_result(r);

    // 5. mixed with standard specifiers
    printf("\n5) \"id=42 year=XIV name=bob 3.5\" with \"id=%%d year=%%Ro name=%%s %%lf\"\n");
    int id = 0, year = 0;
    char name[16] = "";
    double d = 0;
    r = oversscanf("id=42 year=XIV name=bob 3.5", "id=%d year=%Ro name=%15s %lf", &id, &year, name, &d);
    print_scan_result(r);
    printf("Values: %d %d %s %.1f\n", id, year, name, d);

    // 6. stops at the first bad value
    printf("\n6) \"XIV abc 5\" with \"%%Ro %%Ro %%d\"\n");
    int p1 = 0, p2 = 0, p3 = 0;
    r = oversscanf("XIV abc 5", "%Ro %Ro %d", &p1, &p2, &p3);
    print_scan_result(r);

    // 7. empty input
    printf("\n7) empty string\n");
    r = oversscanf("", "%Ro", &a);
    print_scan_result(r);

    // 8. %n and %*d
    printf("\n8) \"10 20 XIV\" with \"%%*d %%d%%n %%Ro\"\n");
    int v = 0, cnt = 0, rom = 0;
    r = oversscanf("10 20 XIV", "%*d %d%n %Ro", &v, &cnt, &rom);
    print_scan_result(r);
    printf("Values: %d, chars read so far: %d, %d\n", v, cnt, rom);

    // 9. overfscanf from a file
    printf("\n9) overfscanf from a file\n");
    FILE* f = tmpfile();
    if (f == NULL){
        printf("Error: cannot create temporary file\n");
        return 1;
    }
    fputs("7 XLII 1010011 -1f\n", f);
    rewind(f);
    int n7 = 0, n42 = 0, h = 0;
    unsigned int z = 0;
    r = overfscanf(f, "%d %Ro %Zr %Cv", &n7, &n42, &z, &h, 16);
    print_scan_result(r);
    printf("Values: %d %d %u %d\n", n7, n42, z, h);
    fclose(f);

    // 10. errors
    printf("\n10) NULL string, unknown specifier\n");
    print_scan_result(oversscanf(NULL, "%d", &a));
    print_scan_result(oversscanf("1", "%q", &a));

    return 0;
}
