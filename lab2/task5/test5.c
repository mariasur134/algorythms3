#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "functions5.h"

// checks that oversprintf returned strlen(expected) and wrote exactly expected
#define CHECK_OK(expected, ...)                                  \
    do {                                                         \
        char buf[512];                                           \
        int r = oversprintf(buf, __VA_ARGS__);                   \
        assert(r == (int)strlen(expected));                      \
        assert(strcmp(buf, (expected)) == 0);                    \
    } while (0)

// checks that oversprintf failed with the given status
#define CHECK_ERR(status, ...)                                   \
    do {                                                         \
        char buf[512];                                           \
        int r = oversprintf(buf, __VA_ARGS__);                   \
        assert(r == -(int)(status));                             \
    } while (0)

int is_little_endian(void){
    unsigned int x = 1;
    return *(unsigned char*)&x == 1;
}

int main(void){
    // roman
    CHECK_OK("I", "%Ro", 1);
    CHECK_OK("IV", "%Ro", 4);
    CHECK_OK("IX", "%Ro", 9);
    CHECK_OK("XLII", "%Ro", 42);
    CHECK_OK("MCMXCIV", "%Ro", 1994);
    CHECK_OK("MMMCMXCIX", "%Ro", 3999);
    CHECK_OK("MMMDCCCLXXXVIII", "%Ro", 3888);   // the longest one (15 chars)
    CHECK_ERR(STATUS_ERR_RANGE, "%Ro", 0);
    CHECK_ERR(STATUS_ERR_RANGE, "%Ro", -5);
    CHECK_ERR(STATUS_ERR_RANGE, "%Ro", 4000);

    // zeckendorf
    CHECK_OK("11", "%Zr", 1u);
    CHECK_OK("011", "%Zr", 2u);
    CHECK_OK("0011", "%Zr", 3u);
    CHECK_OK("1011", "%Zr", 4u);
    CHECK_OK("1010011", "%Zr", 17u);
    CHECK_ERR(STATUS_ERR_RANGE, "%Zr", 0u);
    {
        // biggest unsigned int: result must fit in ZR_MAX and have no "11" before the end
        char buf[ZR_MAX];
        assert(zeckendorf_str(UINT_MAX, buf) == STATUS_OK);
        size_t n = strlen(buf);
        assert(buf[n - 1] == '1' && buf[n - 2] == '1');
        for (size_t i = 0; i + 3 < n; i++) assert(!(buf[i] == '1' && buf[i + 1] == '1'));
    }

    // %Cv / %CV
    CHECK_OK("ff", "%Cv", 255, 16);
    CHECK_OK("FF", "%CV", 255, 16);
    CHECK_OK("11111111", "%Cv", 255, 2);
    CHECK_OK("-11111111", "%Cv", -255, 2);
    CHECK_OK("0", "%Cv", 0, 16);
    CHECK_OK("z", "%Cv", 35, 36);
    CHECK_OK("Z", "%CV", 35, 36);
    CHECK_OK("255", "%Cv", 255, 99);   // wrong base -> 10
    CHECK_OK("255", "%Cv", 255, 1);
    CHECK_OK("-80000000", "%Cv", INT_MIN, 16);
    CHECK_OK("-10000000000000000000000000000000", "%Cv", INT_MIN, 2);
    CHECK_OK("7fffffff", "%Cv", INT_MAX, 16);

    // %to / %TO
    CHECK_OK("255", "%to", "ff", 16);
    CHECK_OK("255", "%TO", "FF", 16);
    CHECK_OK("-5", "%to", "-101", 2);
    CHECK_OK("1295", "%to", "zz", 36);
    CHECK_OK("12", "%to", "12", 99);   // wrong base -> 10
    CHECK_OK("0", "%to", "0", 2);
    CHECK_OK("2147483647", "%to", "7fffffff", 16);
    CHECK_OK("-2147483648", "%to", "-80000000", 16);
    CHECK_ERR(STATUS_ERR_DIGIT, "%to", "FF", 16);   // upper case in %to
    CHECK_ERR(STATUS_ERR_DIGIT, "%TO", "ff", 16);   // lower case in %TO
    CHECK_ERR(STATUS_ERR_DIGIT, "%to", "102", 2);   // digit >= base
    CHECK_ERR(STATUS_ERR_DIGIT, "%to", "", 10);
    CHECK_ERR(STATUS_ERR_DIGIT, "%to", "-", 10);
    CHECK_ERR(STATUS_ERR_DIGIT, "%to", "1 2", 10);
    CHECK_ERR(STATUS_ERR_OVERFLOW, "%to", "80000000", 16);
    CHECK_ERR(STATUS_ERR_OVERFLOW, "%to", "-80000001", 16);
    CHECK_ERR(STATUS_ERR_OVERFLOW, "%to", "99999999999999999999999999", 10);
    CHECK_ERR(STATUS_ERR_NULL, "%to", (const char*)NULL, 10);

    // round trip: %Cv then %to
    {
        char tmp[BASE_MAX];
        for (int base = 2; base <= 36; base++){
            int vals[5] = {0, 1, -1, 123456789, INT_MIN};
            for (int i = 0; i < 5; i++){
                int back = 0;
                assert(int_to_base(vals[i], base, 0, tmp) == STATUS_OK);
                assert(base_to_int(tmp, base, 0, &back) == STATUS_OK);
                assert(back == vals[i]);
            }
        }
    }

    // memory dump
    CHECK_OK("11111111 11111111 11111111 11111111", "%mu", 4294967295u);
    CHECK_OK("00000000 00000000 00000000 00000000", "%mi", 0);
    CHECK_OK("11111111 11111111 11111111 11111111", "%mi", -1);
    CHECK_OK("00000000 00000000 10000000 00111111", "%mf", 1.0f);
    if (is_little_endian()){
        CHECK_OK("00000101 00000000 00000000 00000000", "%mi", 5);
        CHECK_OK("00000000 00000000 00000000 00000000 00000000 00000000 11110000 00111111", "%md", 1.0);
    }

    // standard specifiers still work
    CHECK_OK("42", "%d", 42);
    CHECK_OK("   42|42   |00042", "%5d|%-5d|%05d", 42, 42, 42);
    CHECK_OK("-7 +7", "%d %+d", -7, 7);
    CHECK_OK("ff FF 377 0xff", "%x %X %o %#x", 255, 255, 255, 255);
    CHECK_OK("4294967295", "%u", 4294967295u);
    CHECK_OK("123456789012", "%ld", 123456789012L);
    CHECK_OK("-9223372036854775807", "%lld", -9223372036854775807LL);
    CHECK_OK("44", "%hhd", 300);                  // cut to signed char
    CHECK_OK("text|ab|   ab", "%s|%.2s|%5.2s", "text", "abc", "abc");
    CHECK_OK("(null)", "%s", (const char*)NULL);
    CHECK_OK("Z|  Z", "%c|%3c", 'Z', 'Z');
    CHECK_OK("3.14|003.1|3.141590", "%.2f|%05.1f|%f", 3.14159, 3.14159, 3.14159);
    CHECK_OK("1.5e+00 1.5", "%.1e %g", 1.5, 1.5);
    CHECK_OK("2.50", "%.2Lf", (long double)2.5L);
    CHECK_OK("100%", "%d%%", 100);
    CHECK_OK("     7|7     |2.72", "%*d|%-*d|%.*f", 6, 7, 6, 7, 2, 2.71828);
    CHECK_OK("7|abc", "%*d|%.*s", -1, 7, -1, "abc");   // negative '*' precision is ignored
    CHECK_OK("", "%s", "");
    CHECK_OK("no specifiers", "no specifiers");

    // %n
    {
        char buf[64];
        int cnt = -1;
        int r = oversprintf(buf, "abc%nde", &cnt);
        assert(r == 5 && cnt == 3 && strcmp(buf, "abcde") == 0);
    }

    // our and standard specifiers in one format (the order of arguments on the stack matters)
    CHECK_OK("A XIV B ff C 26 D 7", "A %Ro B %Cv C %to D %d", 14, 255, 16, "1a", 16, 7);
    CHECK_OK("x=3 FF 1011 y=1.50", "x=%d %CV %Zr y=%.2f", 3, 255, 16, 4u, 1.5);

    // long output: the buffer must grow
    {
        char big[2048];
        char* src = (char*)malloc(1500);
        assert(src != NULL);
        for (int i = 0; i < 1499; i++) src[i] = 'a' + (i % 26);
        src[1499] = '\0';
        int r = oversprintf(big, "%s %Ro", src, 14);
        assert(r == 1499 + 1 + 3);
        assert(strcmp(big + 1500, "XIV") == 0);
        free(src);
    }

    // errors
    CHECK_ERR(STATUS_ERR_FORMAT, "%q", 1);
    CHECK_ERR(STATUS_ERR_FORMAT, "abc%");
    CHECK_ERR(STATUS_ERR_FORMAT, "%lc", 1);
    CHECK_ERR(STATUS_ERR_FORMAT, "%1234567890123456789012345678901234567890123456789d", 1);
    {
        char buf[16];
        assert(oversprintf(NULL, "x") == -(int)STATUS_ERR_NULL);
        assert(oversprintf(buf, NULL) == -(int)STATUS_ERR_NULL);
        assert(overfprintf(NULL, "x") == -(int)STATUS_ERR_NULL);
    }

    // overfprintf into a real file
    {
        FILE* f = tmpfile();
        assert(f != NULL);
        int r = overfprintf(f, "n=%d %Ro %Cv\n", 5, 5, 10, 2);
        assert(r == (int)strlen("n=5 V 1010\n"));
        rewind(f);
        char line[64];
        assert(fgets(line, sizeof line, f) != NULL);
        assert(strcmp(line, "n=5 V 1010\n") == 0);
        fclose(f);
    }

    printf("All tests passed\n");
    return 0;
}
