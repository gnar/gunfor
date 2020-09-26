#ifndef UNTITLED2_DICT_H
#define UNTITLED2_DICT_H

#include "memory.h"

extern u64 last;

static const u64 HIDDEN_FLAG = (1ul << 63u);
static const u64 IMMEDIATE_FLAG = (1ul << 62u);
static const u64 MASK = ~(HIDDEN_FLAG | IMMEDIATE_FLAG);

void create(const char *text, int is_immediate);
u64 find(const char *text);
u64 to_tca(u64 addr);
int is_immediate(u64 addr);

#endif
