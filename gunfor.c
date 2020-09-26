/*
 * gunfor, a Forth-like interpreter.
 * github.com/gnar/gunfor
 *
 * This code will only work on architectures where sizeof(void*) == sizeof(unsigned long int).
 *
 * Inspired by jonesforth (github.com/nornagon/jonesforth).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define HEAP_SIZE 32768
#define RETURN_STACK_SIZE 64
#define DATA_STACK_SIZE 1024

// -- global state vars --
typedef char u8;
typedef unsigned long int u64;
static_assert(sizeof(void *) == sizeof(u64), "pointer size mismatch");

typedef void(*code_t)(); // codeword: a pointer to a builtin operation
typedef code_t *instr_t; // instr_t: an 'instruction', i.e. a pointer to a codeword

static int quit_flag = 0;
static int state = 0; // 0=interpreting 1=compiling

static u64 heap_start = 0, heap_end = 0;
static u64 here = 0;

static instr_t *r_stack[RETURN_STACK_SIZE] = {0};
static u64 rsp = 0;
static instr_t *ip = NULL; // instruction pointer

static u64 d_stack[DATA_STACK_SIZE];
static u64 dsp = 0;

static u64 latest = 0;

// --- heap ---
static u64 allot(u64 n) {
    u64 tmp = here;
    here += n;
    return tmp;
}

static char peek1(u64 addr) {
    u8 *string = (u8 *) addr;
    return *string;
}

static void poke1(u64 addr, u8 value) {
    *(u8 *) addr = value;
}

static u64 peek(u64 addr) {
    return *(u64 *) addr;
}

static void poke(u64 addr, u64 value) {
    *(u64 *) addr = value;
}

static u64 make_aligned(u64 addr) {
    return (addr + sizeof(u64) - 1) & ~(sizeof(u64) - 1);
}

static void align() {
    here = make_aligned(here);
}

static void append(u64 value) {
    poke(here, value);
    here += sizeof(u64);
}

static void append1(u8 value) {
    poke1(here, value);
    here += 1;
}

// --- return stack ---
static void renter(instr_t *ip1) {
    r_stack[rsp++] = ip;
    ip = ip1;
}

static void rleave() {
    ip = r_stack[--rsp];
}

// --- data stack ---
static void push(u64 value) {
    d_stack[dsp++] = value;
}

static u64 pop() {
    return d_stack[--dsp];
}

// --- dictionary ---
static const u64 HIDDEN_FLAG = (1ul << 63u);
static const u64 IMMEDIATE_FLAG = (1ul << 62u);
static const u64 MASK = ~(HIDDEN_FLAG | IMMEDIATE_FLAG);

static void create(const char *text, int is_immediate) {
    u64 len = strlen(text);
    align();
    u64 last = latest;
    latest = here;
    append(last);
    append(len | (is_immediate ? IMMEDIATE_FLAG : 0));
    for (int i = 0; i < len; ++i) {
        append1(text[i]);
    }
    align();
}

static u64 find(const char *text) {
    u64 len = strlen(text);
    u64 *it = (u64 *) latest;
    while (it) {
        u64 *prev = (u64 *) it[0];
        u64 len1 = it[1] & MASK;
        const char *text1 = (const char *) &it[2];
        if (len == len1 && 0 == strncmp(text, text1, len1)) {
            return (u64) it;
        }
        it = prev;
    }
    return 0;
}

static u64 to_tca(u64 addr) {
    u64 len = ((u64 *) addr)[1] & MASK;
    return (u64) make_aligned(addr + 2 * sizeof(u64) + len);
}

static int is_immediate(u64 addr) {
    return ((u64 *) addr)[1] & IMMEDIATE_FLAG ? 1 : 0;
}

static void append_tca(const char *word) {
    u64 addr = find(word);
    assert(addr);
    append(to_tca(addr));
}

static void append_tca_with_val(const char *word, u64 value) {
    append_tca(word);
    append(value);
}

// --- builtins ---

static void docol(instr_t target) {
    instr_t data = &target[1];
    renter((instr_t *) (&data[0]));
}

static void f_docol() {
    instr_t target = ip[-1];
    assert(*target == &f_docol);
    docol(target);
}

static void f_exit() {
    rleave();
}

static void f_branch() {
    ip = (instr_t *) ((u64) ip + peek((u64) ip));
}

static void f_0branch() {
    if (0 == pop()) {
        f_branch();
    }
}

static void f_lit() {
    push((u64) *ip++);
}

static void f_immediate_mode() { // [
    state = 0;
}

static void f_compile_mode() { // ]
    state = 1;
}

static void f_peek() { // @
    assert(0);
}

static void f_append() { // ,
    u64 a = pop();
    append(a);
}

static void f_dup() {
    u64 a = pop();
    push(a);
    push(a);
}

static void f_drop() {
    pop();
}

static void f_swap() {
    u64 a = pop();
    u64 b = pop();
    push(a);
    push(b);
}

static void f_print() {
    long a = (long) pop();
    printf("%ld", a);
}

static void f_add() {
    u64 a = pop();
    u64 b = pop();
    push(a + b);
}

static void f_bye() {
    quit_flag = 1;
}

static u64 word(u64 *len) {
    static char buf[1024] = {0};
    scanf("%s", buf);
    *len = strlen(buf);
    return (u64) buf;
}

static void f_word() {
    u64 len;
    u64 text = word(&len);
    push(text);
    push(len);
}

static void f_create() {
    pop(); // len
    u64 text = pop();
    create((const char *) text, 0);
}

static void f_info() {
    printf("words:");
    for (u64 *it = (u64 *) latest; it; it = (u64 *) it[0]) {
        char *text = strndup((const char *) &it[2], it[1]);
        printf(" %s", text);
        free(text);
    }
    printf("\n");
    printf("heap: %lu bytes allotted, %lu bytes free.\n", here - heap_start, heap_end - here);
    printf("stack: <%lu> ", dsp);
    for (u64 i = 0; i < dsp; ++i) {
        printf(" %lu", d_stack[i]);
    }
    printf("\n");
}

static void f_interpret() {
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

static void define_builtin(const char *word, code_t fn, int is_immediate) {
    create(word, is_immediate);
    append((u64) fn);
}

int main() {
    heap_start = (u64) malloc(HEAP_SIZE);
    heap_end = heap_start + HEAP_SIZE;
    here = (u64) heap_start;

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
    define_builtin("drop", f_drop, 0);
    define_builtin("swap", f_swap, 0);
    define_builtin("+", f_add, 0);
    define_builtin(".", f_print, 0);
    define_builtin("info", f_info, 0);

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

    free((void *) heap_start);
    return 0;
}
