#include "kernel.h"

int strings_match(const char *s1, const char *s2) {
    while (*s1 != '\0' && *s1 == *s2) {
        ++s1;
        ++s2;
    }
    return *s1 == *s2;
}

int StringPrefixMatch(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++;
        prefix++;
    }
    return 1;
}
