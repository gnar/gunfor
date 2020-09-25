#include "interpret.h"

instr_t *ip;
static instr_t *rstack[64] = {0};
static int rsp = 0;

void renter(instr_t *ip1) {
    rstack[rsp++] = ip;
    ip = ip1;
}

void rleave() {
    ip = rstack[--rsp];
}
