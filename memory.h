#ifndef UNTITLED2_MEMORY_H
#define UNTITLED2_MEMORY_H

#include <stdint.h>

typedef char u8;
typedef unsigned long int u64;

extern u64 here;

u64 aligned(u64 addr);

char peek1(u64 addr);
void poke1(u64 addr, u8 value);
u64 peek(u64 addr);
void poke(u64 addr, u64 value);

u64 align();
u64 allot(u64 n);
u64 append(u64 value);
u64 append1(u8 value);

#endif
