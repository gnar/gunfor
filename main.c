#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "memory.h"
#include "dict.h"
#include "stack.h"

typedef void(*code_t)(); // codeword: a pointer to a builtin operation
typedef code_t* instr_t; // instr_t: an 'instruction', i.e. a pointer to a codeword

// -- global state vars --
static instr_t *ip;

// --- return stack ---

static instr_t *rstack[64] = {0};
static int rsp = 0;

void renter(instr_t *ip1) {
    rstack[rsp++] = ip;
    ip = ip1;
}

void rleave() {
    ip = rstack[--rsp];
}

int state = 0; // 0=interpreting 1=compiling
int quit_flag = 0;

void docol(instr_t target) {
    instr_t data = &target[1];
    renter((instr_t *) (&data[0]));
}

void f_docol() {
    instr_t target = ip[-1];
    assert(*target == &f_docol);
    docol(target);
}

void f_exit() {
    rleave();
}

void f_branch() {
    ip = (instr_t *) ((u64) ip + peek((u64) ip));
}

void f_0branch() {
    if (0 == pop()) {
        f_branch();
    }
}

void f_lit() {
    push((u64) *ip++);
}

void f_immediate_mode() { // [
    state = 0;
}

void f_compile_mode() { // ]
    state = 1;
}

void f_peek() { // @
    assert(0);
}

void f_append() { // ,
    u64 a = pop();
    append(a);
}

void f_dup() {
    u64 a = pop();
    push(a);
    push(a);
}

void f_swap() {
    u64 a = pop();
    u64 b = pop();
    push(a);
    push(b);
}

void f_print() {
    long a = (long) pop();
    printf("%ld", a);
}

void f_add() {
    u64 a = pop();
    u64 b = pop();
    push(a + b);
}

void f_bye() {
    quit_flag = 1;
}

u64 word(u64 *len) {
    static char buf[1024] = {0};
    scanf("%s", buf);
    *len = strlen(buf);
    return (u64) buf;
}

void f_word() {
    u64 len;
    u64 text = word(&len);
    push(text);
    push(len);
}

void f_create() {
    pop(); // len
    u64 text = pop();
    create((const char *) text, 0);
}

void f_interpret() {
    u64 n = 0;
    const char *text = (const char *) word(&n);

    u64 addr = find(text);
    if (addr) {
        // word
        if (state == 0 || is_immediate(addr)) {
            code_t code = *(code_t *) to_tca(addr);
            if (code == f_docol) {
                docol((instr_t) to_tca(addr));
            } else {
                code();
            }
        } else {
            // compile
            append(to_tca(addr));
        }
    } else {
        // literal?
        char *end;
        u64 value = strtol(text, &end, 10);
        if (0 != strlen(end)) {
            fprintf(stderr, "parse error: not a word or literal\n");
            fflush(stderr);
        } else if (state == 0) {
            // interpret
            push(value);
        } else {
            // compile
            static const code_t lit = f_lit;
            append((u64) &lit);
            append(value);
        }
    }
}

void define_builtin(const char *word, code_t fn, int is_immediate) {
    create(word, is_immediate);
    append((u64) fn);
}

void append_tca(const char *word) {
    u64 addr = find(word);
    assert(addr);
    append(to_tca(addr));
}

void append_tca_with_val(const char *word, u64 value) {
    append_tca(word);
    append(value);
}

int main() {
    define_builtin("exit", f_exit, 0);
    define_builtin("branch", f_branch, 0);
    define_builtin("0branch", f_0branch, 0);
    define_builtin("[", f_immediate_mode, 1);
    define_builtin("]", f_compile_mode, 0);
    define_builtin("@", f_peek, 0);
    define_builtin(",", f_append, 0);
    define_builtin("word", f_word, 0);
    define_builtin("create", f_create, 0);
    define_builtin("interpret", f_interpret, 0);
    define_builtin("bye", f_bye, 0);
    define_builtin("lit", f_lit, 0);
    define_builtin("dup", f_dup, 0);
    define_builtin("swap", f_swap, 0);
    define_builtin("+", f_add, 0);
    define_builtin(".", f_print, 0);

    define_builtin("quit", f_docol, 0);
    append_tca("interpret");
    append_tca_with_val("branch", -16);

    define_builtin(":", f_docol, 0);
    append_tca("word");
    append_tca("create");
    append_tca_with_val("lit", (u64) f_docol);
    append_tca(",");
    // TODO: hide latest
    append_tca("]");
    append_tca("exit");

    define_builtin(";", f_docol, 1);
    append_tca_with_val("lit", to_tca(find("exit")));
    append_tca(",");
    // TODO: unhide latest
    append_tca("[");
    append_tca("exit");

    static instr_t coldstart;
    coldstart = (instr_t) to_tca(find("quit"));

    ip = &coldstart;
    while (!quit_flag) {
        instr_t instr = *ip++;
        code_t code = *instr;
        code();
    }

    return 0;
}
