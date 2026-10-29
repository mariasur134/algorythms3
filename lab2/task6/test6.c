#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "functions6.h"

int main(void){
    // %Ro
    {
        int v = 0;
        assert(oversscanf("I", "%Ro", &v) == 1 && v == 1);
        assert(oversscanf("IV", "%Ro", &v) == 1 && v == 4);
        assert(oversscanf("IX", "%Ro", &v) == 1 && v == 9);
        assert(oversscanf("XLII", "%Ro", &v) == 1 && v == 42);
        assert(oversscanf("MCMXCIV", "%Ro", &v) == 1 && v == 1994);
        assert(oversscanf("MMMCMXCIX", "%Ro", &v) == 1 && v == 3999);
        assert(oversscanf("   \n XIV rest", "%Ro", &v) == 1 && v == 14);   // leading spaces are skipped
        v = 77;
        assert(oversscanf("abc", "%Ro", &v) == 0 && v == 77);               // not a roman digit: nothing stored
        assert(oversscanf("xiv", "%Ro", &v) == 0 && v == 77);               // only upper case
        assert(oversscanf("", "%Ro", &v) == EOF);
        assert(oversscanf("   ", "%Ro", &v) == EOF);
    }

    // %Zr
    {
        unsigned int v = 0;
        assert(oversscanf("11", "%Zr", &v) == 1 && v == 1);
        assert(oversscanf("011", "%Zr", &v) == 1 && v == 2);
        assert(oversscanf("0011", "%Zr", &v) == 1 && v == 3);
        assert(oversscanf("1011", "%Zr", &v) == 1 && v == 4);
        assert(oversscanf("1010011", "%Zr", &v) == 1 && v == 17);
        assert(oversscanf("1010011 tail", "%Zr", &v) == 1 && v == 17);       // stops at the end marker
        assert(oversscanf("10100112", "%Zr", &v) == 1 && v == 17);
        v = 5;
        assert(oversscanf("1010", "%Zr", &v) == 0 && v == 5);               // no end marker
        assert(oversscanf("x", "%Zr", &v) == 0);
        assert(oversscanf("", "%Zr", &v) == EOF);
        // the 47th fibonacci number (4807526976) is bigger than UINT_MAX
        assert(oversscanf("000000000000000000000000000000000000000000000011", "%Zr", &v) == 0);
        // too long token
        char longtok[100];
        memset(longtok, '0', sizeof longtok - 1);
        longtok[sizeof longtok - 1] = '\0';
        assert(oversscanf(longtok, "%Zr", &v) == 0);
    }

    // %Cv / %CV
    {
        int v = 0;
        assert(oversscanf("ff", "%Cv", &v, 16) == 1 && v == 255);
        assert(oversscanf("FF", "%CV", &v, 16) == 1 && v == 255);
        assert(oversscanf("-101", "%Cv", &v, 2) == 1 && v == -5);
        assert(oversscanf("zz", "%Cv", &v, 36) == 1 && v == 1295);
        assert(oversscanf("ZZ", "%CV", &v, 36) == 1 && v == 1295);
        assert(oversscanf("255", "%Cv", &v, 99) == 1 && v == 255);           // wrong base -> 10
        assert(oversscanf("255", "%Cv", &v, 1) == 1 && v == 255);
        assert(oversscanf("0", "%Cv", &v, 2) == 1 && v == 0);
        assert(oversscanf("7fffffff", "%Cv", &v, 16) == 1 && v == INT_MAX);
        assert(oversscanf("-80000000", "%Cv", &v, 16) == 1 && v == INT_MIN);
        assert(oversscanf("12 rest", "%Cv", &v, 10) == 1 && v == 12);
        assert(oversscanf("1012", "%Cv", &v, 2) == 1 && v == 5);             // '2' is not a digit in base 2: reading stops
        v = 9;
        assert(oversscanf("FF", "%Cv", &v, 16) == 0 && v == 9);              // upper case in %Cv
        assert(oversscanf("ff", "%CV", &v, 16) == 0 && v == 9);              // lower case in %CV
        assert(oversscanf("-", "%Cv", &v, 10) == 0);
        assert(oversscanf("80000000", "%Cv", &v, 16) == 0);                  // does not fit in int
        assert(oversscanf("99999999999999999999999999999999999999999999", "%Cv", &v, 10) == 0);
        assert(oversscanf("", "%Cv", &v, 10) == EOF);
    }

    // standard specifiers still work
    {
        int a = 0, b = 0;
        char s[32];
        double d = 0;
        long l = 0;
        char ch = 0;
        assert(oversscanf("12 34", "%d %d", &a, &b) == 2 && a == 12 && b == 34);
        assert(oversscanf("12,34", "%d,%d", &a, &b) == 2 && a == 12 && b == 34);
        assert(oversscanf("  hello world", "%31s", s) == 1 && strcmp(s, "hello") == 0);
        assert(oversscanf("2.5", "%lf", &d) == 1 && d == 2.5);
        assert(oversscanf("123456789012", "%ld", &l) == 1 && l == 123456789012L);
        assert(oversscanf("ff", "%x", &a) == 1 && a == 255);
        assert(oversscanf("12345", "%3d", &a) == 1 && a == 123);
        assert(oversscanf("abc", "%c", &ch) == 1 && ch == 'a');
        assert(oversscanf("abc123", "%[a-z]", s) == 1 && strcmp(s, "abc") == 0);
        assert(oversscanf("abc123", "%[^0-9]", s) == 1 && strcmp(s, "abc") == 0);
        assert(oversscanf("50%", "%d%%", &a) == 1 && a == 50);
        assert(oversscanf("1 2", "%*d %d", &a) == 1 && a == 2);              // suppressed: not stored, not counted
        assert(oversscanf("x", "%d", &a) == 0);
        assert(oversscanf("", "%d", &a) == EOF);
        assert(oversscanf("1", "%d %d", &a, &b) == 1);                       // second value missing
    }

    // %n
    {
        int a = 0, n = -1;
        assert(oversscanf("12 34", "%d%n", &a, &n) == 1 && n == 2);
        assert(oversscanf("XIV 5", "%Ro%n", &a, &n) == 1 && n == 3);
        int b = 0;
        assert(oversscanf("XIV 5", "%Ro %d%n", &a, &b, &n) == 2 && n == 5);
    }

    // our and standard specifiers together (the order of arguments matters)
    {
        int year = 0, num = 0, hex = 0, id = 0;
        unsigned int z = 0;
        char word[16];
        assert(oversscanf("id=7 year=MMXXIV z=1010011 hex=ff w=abc", "id=%d year=%Ro z=%Zr hex=%Cv w=%15s",
                          &id, &year, &z, &hex, 16, word) == 5);
        assert(id == 7 && year == 2024 && z == 17 && hex == 255 && strcmp(word, "abc") == 0);

        assert(oversscanf("XIV abc 5", "%Ro %Ro %d", &year, &num, &id) == 1);   // stops at the first bad value
        assert(year == 14);
    }

    // literal text in the format
    {
        int a = 0;
        assert(oversscanf("x=5", "x=%d", &a) == 1 && a == 5);
        assert(oversscanf("y=5", "x=%d", &a) == 0);
        assert(oversscanf("x", "x=%d", &a) == EOF);
    }

    // errors of the call
    {
        int a = 0;
        assert(oversscanf(NULL, "%d", &a) == -(int)STATUS_ERR_NULL);
        assert(oversscanf("1", NULL) == -(int)STATUS_ERR_NULL);
        assert(oversscanf("1", "%d", NULL) == -(int)STATUS_ERR_NULL);
        assert(oversscanf("1", "%Ro", NULL) == -(int)STATUS_ERR_NULL);
        assert(oversscanf("1", "%q", &a) == -(int)STATUS_ERR_FORMAT);
        assert(oversscanf("1", "%", &a) == -(int)STATUS_ERR_FORMAT);
        assert(oversscanf("1", "%[abc", &a) == -(int)STATUS_ERR_FORMAT);
        assert(overfscanf(NULL, "%d", &a) == -(int)STATUS_ERR_NULL);
    }

    // overfscanf: a file, several calls in a row keep the position
    {
        FILE* f = tmpfile();
        assert(f != NULL);
        fputs("7 XLII 1010011 -1f\nlast\n", f);
        rewind(f);

        int n7 = 0, n42 = 0, h = 0;
        unsigned int z = 0;
        char word[16];
        assert(overfscanf(f, "%d", &n7) == 1 && n7 == 7);
        assert(overfscanf(f, "%Ro", &n42) == 1 && n42 == 42);
        assert(overfscanf(f, "%Zr %Cv", &z, &h, 16) == 2 && z == 17 && h == -31);
        assert(overfscanf(f, "%15s", word) == 1 && strcmp(word, "last") == 0);
        assert(overfscanf(f, "%Ro", &n7) == EOF);
        fclose(f);

        // all in one call, with %n
        f = tmpfile();
        assert(f != NULL);
        fputs("7 XLII 1010011 -1f", f);
        rewind(f);
        int n = 0;
        assert(overfscanf(f, "%d %Ro %Zr %Cv%n", &n7, &n42, &z, &h, 16, &n) == 4);
        assert(n7 == 7 && n42 == 42 && z == 17 && h == -31 && n == 18);
        fclose(f);
    }

    printf("All tests passed\n");
    return 0;
}
