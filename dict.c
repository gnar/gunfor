#include <string.h>
#include "dict.h"

u64 last = 0;

void create(const char *text) {
    u64 len = strlen(text);
    align();
    last = append(last);
    append(len);
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
        u64 len1 = it[1];
        const char *text1 = (const char *) &it[2];
        if (len == len1 && 0 == strcmp(text, text1)) {
            return (u64) it;
        }
        it = prev;
    }
    return 0;
}

u64 to_tca(u64 a) {
    u64 len = ((u64 *) a)[1];
    return (u64) aligned(a + 2 * sizeof(u64) + len);
}
