#ifndef UNTITLED2_INTERPRET_H
#define UNTITLED2_INTERPRET_H

#include "memory.h"

typedef void(*code_t)(); // codeword: a pointer to a builtin operation
typedef code_t* instr_t; // instr_t: an 'instruction', i.e. a pointer to a codeword

extern instr_t *ip; // instruction pointer: points to an instruction

void renter(instr_t *ip1);
void rleave();

#endif
