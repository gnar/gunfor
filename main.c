#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "memory.h"
#include "dict.h"
#include "stack.h"
#include "interpret.h"

int state = 0; // 0=interpreting 1=compiling
int quit_flag = 0;

void f_docol() {
    instr_t target = *(ip - 1);
    code_t code = target[0]; // N.B.: code == &f_docol
    (void) code;
    instr_t data = &target[1];
    renter((instr_t *) (&data[0]));
}

void f_exit() {
    rleave();
}

void f_jump() {
    ip = (instr_t *) peek((u64) ip);
}

void f_lit() {
    push((u64) *ip);
    ip += 1;
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
    u64 a = pop();
    printf("%lu", a);
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

void f_interpret() {

    u64 n = 0;
    const char *text = (const char *) word(&n);

    u64 addr = find(text);
    if (addr) {
        // word
        if (state == 0) {
            // interpret
            instr_t *save_ip = ip;
            code_t *op = (code_t*) to_tca(addr);
            (*op)();
            ip = save_ip;
        } else {
            // compile
            append(addr);
        }
    } else {
        // literal?
        char *end;
        u64 value = strtol(text, &end, 10);
        if (*end != '\0') {
            fprintf(stderr, "parse error: not a word or literal\n");
            fflush(stderr);
        } else if (state == 0) {
            // interpret
            push(value);
        } else {
            // compile
            static code_t lit = f_lit;
            append((u64) &lit);
            append(value);
        }
    }
}

void create_builtin(const char *word, code_t fn) {
    create(word);
    append((u64) fn);
}

int main() {
    create_builtin("docol", f_docol);
    create_builtin("interpret", f_interpret);
    create_builtin("exit", f_exit);
    create_builtin("lit", f_lit);
    create_builtin("dup", f_dup);
    create_builtin("swap", f_swap);
    create_builtin("jump", f_jump);
    create_builtin("+", f_add);
    create_builtin(".", f_print);

    create_builtin("double", f_docol);
    append(to_tca(find("dup")));
    append(to_tca(find("+")));
    append(to_tca(find("exit")));

    instr_t repl[] = {
            (instr_t) to_tca(find("interpret")),
            (instr_t) to_tca(find("jump")),
            (instr_t) &repl[0],
            NULL,
    };

    ip = &repl[0];

    while (!quit_flag) {
        instr_t instr = *ip++;
        code_t code = *instr;
        code();
    }

    return 0;
}
