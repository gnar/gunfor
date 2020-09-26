#include "stack.h"

static u64 stack[1024];
static u64 sp = 0;

void push(u64 value) {
    stack[sp++] = value;
}

u64 pop() {
    return stack[--sp];
}

