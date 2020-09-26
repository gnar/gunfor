#include <string.h>
#include "dict.h"

u64 last = 0;

void create(const char *text, int is_immediate) {
    u64 len = strlen(text);
    align();
    last = append(last);
    append(len | (is_immediate ? IMMEDIATE_FLAG : 0));
    for (int i = 0; i < len; ++i) {
        append1(text[i]);
    }
    align();
}

u64 find(const char *text) {
    u64 len = strlen(text);
    u64 *it = (u64 *) last;
    while (it) {
        u64 *prev = (u64 *) it[0];
        u64 len1 = it[1] & MASK;
        const char *text1 = (const char *) &it[2];
        if (len == len1 && 0 == strcmp(text, text1)) {
            return (u64) it;
        }
        it = prev;
    }
    return 0;
}

u64 to_tca(u64 addr) {
    u64 len = ((u64 *) addr)[1] & MASK;
    return (u64) aligned(addr + 2 * sizeof(u64) + len);
}

int is_immediate(u64 addr) {
    return ((u64 *) addr)[1] & IMMEDIATE_FLAG ? 1 : 0;
}
