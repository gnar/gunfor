#include "memory.h"

static char mem[16 * 1024] = {0};

u64 here = (u64) &mem[0];

char peek1(u64 addr) {
    u8 *string = (u8 *) addr;
    return *string;
}

void poke1(u64 addr, u8 value) {
    *(u8 *) addr = value;
}

u64 peek(u64 addr) {
    return *(u64 *) addr;
}

void poke(u64 addr, u64 value) {
    *(u64 *) addr = value;
}

u64 allot(u64 n) {
    u64 tmp = here;
    here += n;
    return tmp;
}

u64 aligned(u64 addr) {
    return (addr + sizeof(u64) - 1) & ~(sizeof(u64) - 1);
}

u64 align() {
    here = aligned(here);
    return here;
}

u64 append(u64 value) {
    u64 tmp = here;
    poke(tmp, value);
    here += sizeof(u64);
    return tmp;
}

u64 append1(u8 value) {
    u64 tmp = here;
    poke1(tmp, value);
    here += 1;
    return tmp;
}
