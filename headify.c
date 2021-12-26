/*
@author: Michael Rohs
@date: December 6, 2021
*/

// https://www.freecodecamp.org/news/how-to-delete-a-git-branch-both-locally-and-remotely/

#include "util.h"
#include "headify.h"

const int DEBUG = false;

/*
Examples:

#include "util.h"
- hash, preproc_begin, preproc_end, preproc, line_break
- hash, preproc, line_break

const int DEBUG = false;
- token+, assign, token+, semicolon, line_break

int i;
- token+, semicolon, line_break

int indentation(String s) {}
- paren_open, paren_close, parens, brace_open, brace_close, braces, line_break
- token+, parens, braces, line_break

*int indentation(String s) {}
- public, token+, parens, braces, line_break
  (public only after newline and whitespace)

int indentation(String s);
- token+, parens, semicolon, line_break

int a[9][9] = {
    { 8, 8, 8, 8, 8, 8, 8, 0, 8 }, 
    ...
};
- token+, brackets+, assign, braces, semicolon, line_break

typedef struct {
    char* owner;
    int balance; }
Account;
- token+, braces, line_break, token, semicolon

struct Point {
    int x;
    int y; };
- token+, braces, semicolon

typedef struct Point Point;
- token+, semicolon
*/

static const char* ElementTypeName[] = { 
    "err", "whi", "tok", "pre", "lco", "bco", "sem", 
    "lbr", "par", "bra", "cur", "clo", "asg", "pub", 
    "eos", "ElementTypeCount"
};

// Generate the ElementList type and its associated functions. See util.h.
generate_list(ElementList, Element, elements_, );

/*
Creates a new dynamically allocated element of the given type extending from
begin (inclusive) to end (exclusive). The next pointer is used to chain elements
in a linked list.
*/
Element* new_element(ElementType type, char* begin, char* end) {
#if 0
    static char* first_begin = NULL;
    if (first_begin == NULL) first_begin = begin;
    int n = end - begin;
    char out[n + 1];
    memcpy(out, begin, n);
    out[n] = '\0';
    printf("new_element: %s, %lu, %lu %s\n", ElementTypeName[type], 
            begin - first_begin, end - first_begin,  type == tok ? out : "");
#endif
    require("valid type", 0 <= type && type < ElementTypeCount);
    require_not_null(begin);
    require_not_null(end);
    require("end not before begin", begin <= end);
    Element* e = xmalloc(sizeof(Element));
    e->type = type;
    e->begin = begin;
    e->end = end;
    e->next = NULL;
    return e;
}

/*
Returns a new element on the stack of the given type extending from begin
(inclusive) to end (exclusive). The next pointer is used to chain elements in a
linked list.
*/
Element make_element(ElementType type, char* begin, char* end) {
    require("valid type", 0 <= type && type < ElementTypeCount);
    require_not_null(begin);
    require_not_null(end);
    require("end not before begin", begin <= end);
    return (Element){type, begin, end, NULL};
}

/*
Prints the type and contents of the element, followed by a line break.
*/
void print_element(Element* e) {
    require_not_null(e);
    printf("%s: %.*s\n", ElementTypeName[e->type], (int)(e->end - e->begin), e->begin);
}

/*
Prints all of the elements in the elements list.
*/
void print_elements(ElementList* list) {
    require_not_null(list);
    for (Element* e = list->first; e != NULL; e = e->next) {
        print_element(e);
    }
}

/*
Checks whether bo is an opening brace character and bc is the corresponding
closing brace character.
*/
bool braces_match(char bo, char bc) {
    return (bo == '(' && bc == ')') 
        || (bo == '{' && bc == '}') 
        || (bo == '[' && bc == ']');
}

/*
Counts the number of line breaks between s (inclusive) and t (exclusive).
*/
int count_lines(char* s, char* t) {
    require_not_null(s);
    require_not_null(t);
    int n = 0;
    for (; s < t; s++) {
        if (*s == '\n') n++;
    }
    return n;
}

// Is the scanner in the indentation region at the beginning of a line?
static bool indent = false;

// Contains the error message in case of an error.
static char* error_message = NULL;

// Contains the error position in case of an error.
static char* error_pos = NULL;

/*
Get next element from source code string.
*/
Element scan_next(char* s) {
    char* t = s + 1;
    char c = *s;
    char d = *t;
    bool escape = false;
    switch (c) {
    case '\0': return make_element(eos, s, s);
    case '\n': indent = true; return make_element(lbr, s, t);
    case '*': return make_element(indent ? pub : tok, s, t);
    case ';': indent = false; return make_element(sem, s, t);
    case '=': indent = false; return make_element(asg, s, t);
    case '#': 
        if (!indent) return make_element(tok, s, t);
        indent = false;
        while (*t != '\0') {
            if (*t == '\\' && *(t + 1) == '\n') {
                t += 2;
                continue;
            }
            if (*t == '\n') {
                break;
            }
            t++;
        }
        return make_element(pre, s, t);
    case ' ': case '\t': 
        while (*t != '\0') {
            if (*t == '\\' && *(t + 1) == '\n') {
                t += 2;
                continue;
            }
            if (*t != ' ' && *t != '\t') {
                break;
            }
            t++;
        }
        return make_element(whi, s, t);
    case '"': 
        indent = false;
        escape = false;
        while (*t != '\0') {
            if (!escape && *t == '"') {
                // treat string literals as tokens
                return make_element(tok, s, t + 1);
            }
            if (*t == '\\') {
                escape = !escape;
            } else {
                escape = false;
            }
            t++;
        }
        error_message = "unterminated string literal";
        error_pos = s;
        return make_element(err, s, t);
    case '\'': 
        indent = false;
         escape = false;
        while (*t != '\0') {
            if (!escape && *t == '\'') {
                // treat character literals as tokens
                return make_element(tok, s, t + 1);
            }
            if (*t == '\\') {
                escape = !escape;
            } else {
                escape = false;
            }
            t++;
        }
        error_message = "unterminated character literal";
        error_pos = s;
        return make_element(err, s, t);
    case '/':
        if (d == '/') {
            indent = false;
            t++;
            while (*t != '\0') {
                /* no nulti-line // comments
                if (*t == '\\' && *(t + 1) == '\n') {
                    t += 2;
                    continue;
                }
                */
                if (*t == '\n') {
                    break;
                }
                t++;
            }
            return make_element(lco, s, t);
        } else if (d == '*') {
            t++;
            while (*t != '\0') {
                if (*t == '*' && *(t + 1) == '/') {
                    return make_element(bco, s, t + 2);
                }
                t++;
            }
            error_message = "unterminated block comment";
            error_pos = s;
            return make_element(err, s, t);
        } else {
            return make_element(tok, s, t);
        }
    case '(': case '{': case '[':
        indent = false;
        while (*t != '\0') {
            Element e = scan_next(t);
            t = e.end;
            if (e.type == eos) break;
            if (e.type == err) return e;
            if (e.type == clo) {
                if (braces_match(c, *e.begin)) {
                    if (c == '(') return make_element(par, s, t);
                    if (c == '{') return make_element(cur, s, t);
                    /* [ */ return make_element(bra, s, t);
                } else {
                    error_message = "braces do not match";
                    error_pos = e.begin;
                    return make_element(err, s, t);
                }
            }
        }
        error_message = "unterminated braces";
        error_pos = s;
        return make_element(err, s, t);
    case ')': case '}': case ']':
        indent = false;
        return make_element(clo, s, t);
    default:
        assert("not eos", c != '\0');
        indent = false;
        while (*t != '\0') {
            switch (*t) {
                case ' ': case '\t': case '\n': 
                case ';': case '=': 
                case '/': case '\\': 
                case '"': case '\'': 
                case '(': case '{': case '[': 
                case ')': case '}': case ']':
                    return make_element(tok, s, t);
            }
            t++;
        }
        return make_element(tok, s, t);
    }
}

/*
Test function for an element.
*/
#define test_equal_element(actual, type, content, follow) \
    base_test_equal_element(__FILE__, __LINE__, actual, type, content, follow)
bool base_test_equal_element(const char* file, int line, Element e, ElementType type, 
        char* content, char* follow) {
    bool ok1 = base_test_equal_s(file, line, make_string2(e.begin, e.end - e.begin), content);
    bool ok2 = base_test_equal_s(file, line, make_string2(e.end, strlen(e.end)), follow);
    bool ok3 = base_test_equal_i(file, line, e.type, type);
    if (!ok3) printf("\t\ttypes: actual = %s, expected = %s\n", 
            ElementTypeName[e.type], ElementTypeName[type]);
    return ok1 && ok2 && ok3;
}

/*
Test scan_next.
*/
void scan_next_test(void) {
    Element e;
    
    e = scan_next("");
    test_equal_element(e, eos, "", "");
    
    e = scan_next("\nabc");
    test_equal_element(e, lbr, "\n", "abc");
    
    indent = true;
    e = scan_next("*abc");
    test_equal_element(e, pub, "*", "abc");
    
    indent = false;
    e = scan_next("*abc");
    test_equal_element(e, tok, "*", "abc");
    
    e = scan_next("abc*");
    test_equal_element(e, tok, "abc*", "");
    
    e = scan_next(";abc");
    test_equal_element(e, sem, ";", "abc");

    e = scan_next("=abc");
    test_equal_element(e, asg, "=", "abc");

    indent = true;
    e = scan_next("#abc");
    test_equal_element(e, pre, "#abc", "");

    indent = false;
    e = scan_next("#abc");
    test_equal_element(e, tok, "#", "abc");

    indent = true;
    e = scan_next("#a\nb");
    test_equal_element(e, pre, "#a", "\nb");

    indent = false;
    e = scan_next("#a\nb");
    test_equal_element(e, tok, "#", "a\nb");
    
    e = scan_next(" \t abc def");
    test_equal_element(e, whi, " \t ", "abc def");
    
    e = scan_next("\"\"x");
    test_equal_element(e, tok, "\"\"", "x");
    
    e = scan_next("\"a\"x");
    test_equal_element(e, tok, "\"a\"", "x");
    
    e = scan_next("\"\\\\\"x");
    test_equal_element(e, tok, "\"\\\\\"", "x");
    
    e = scan_next("''x");
    test_equal_element(e, tok, "''", "x");
    
    e = scan_next("'a'x");
    test_equal_element(e, tok, "'a'", "x");
    
    e = scan_next("'\\\\'x");
    test_equal_element(e, tok, "'\\\\'", "x");

    e = scan_next("//x");
    test_equal_element(e, lco, "//x", "");
    
    e = scan_next("//x\na");
    test_equal_element(e, lco, "//x", "\na");
    
    e = scan_next("/**/x");
    test_equal_element(e, bco, "/**/", "x");
    
    e = scan_next("/*abc*/x");
    test_equal_element(e, bco, "/*abc*/", "x");
    
    e = scan_next("abc def");
    test_equal_element(e, tok, "abc", " def");
    
    e = scan_next("(abc)x");
    test_equal_element(e, par, "(abc)", "x");

    e = scan_next("(x[abc]y)z");
    test_equal_element(e, par, "(x[abc]y)", "z");

    e = scan_next("{x[a{ b }c]y}z");
    test_equal_element(e, cur, "{x[a{ b }c]y}", "z");

    e = scan_next("[x[a{ b }c]y]z");
    test_equal_element(e, bra, "[x[a{ b }c]y]", "z");

    e = scan_next("([abc)x");
    test_equal_element(e, err, "[abc)", "x");
    printf("error = %s\n", error_message);

    e = scan_next("/*abc");
    test_equal_element(e, err, "/*abc", "");
    printf("error = %s\n", error_message);

    e = scan_next("\"abc");
    test_equal_element(e, err, "\"abc", "");
    printf("error = %s\n", error_message);

    e = scan_next("'abc");
    test_equal_element(e, err, "'abc", "");
    printf("error = %s\n", error_message);

    e = scan_next("abc def");
    test_equal_element(e, tok, "abc", " def");

    e = scan_next("abc\tdef");
    test_equal_element(e, tok, "abc", "\tdef");

    e = scan_next("abc\ndef");
    test_equal_element(e, tok, "abc", "\ndef");

    e = scan_next("abc(def");
    test_equal_element(e, tok, "abc", "(def");

    e = scan_next("abc{def");
    test_equal_element(e, tok, "abc", "{def");

    e = scan_next("abc[def");
    test_equal_element(e, tok, "abc", "[def");

    e = scan_next("abc;def");
    test_equal_element(e, tok, "abc", ";def");

    e = scan_next("abc=def");
    test_equal_element(e, tok, "abc", "=def");

    e = scan_next("abc/def");
    test_equal_element(e, tok, "abc", "/def");

    e = scan_next("/def");
    test_equal_element(e, tok, "/", "def");

    indent = true;
}

/*
Parses the source code string into a list of elements.
*/
ElementList get_elements(char* filename, String source_code) {
    require_not_null(filename);
    ElementList elements = {NULL, NULL};
    Element e = scan_next(source_code.s);
    while (e.type != eos) {
        if (e.type == err) {
            int line = count_lines(source_code.s, error_pos) + 1;
            printf("%s:%d: %s\n", filename, line, error_message);
            exit(EXIT_FAILURE);
        }
        elements_append(&elements, new_element(e.type, e.begin, e.end));
        e = scan_next(e.end);
    }
    return elements;
}

#if 0
// Checks if e is a brace.
bool is_bra(Element* e) {
    return e != NULL && e->type == bra;
}

// Checks if e is a token.
bool is_tok(Element* e) {
    return e != NULL && e->type == tok;
}

// Checks if e is a semicolon.
bool is_sem(Element* e) {
    return e != NULL && e->type == sem;
}

// Checks if e is an assignment.
bool is_asg(Element* e) {
    return e != NULL && e->type == asg;
}

// Checks if e is a parentheses element (...).
bool is_paren(Element* e) {
    return e != NULL && e->type == bra && *e->begin == '(';
}

// Checks if e is a brackets element [...].
bool is_bracket(Element* e) {
    return e != NULL && e->type == bra && *e->begin == '[';
}

// Checks if e is a curly braces element {...}.
bool is_curly(Element* e) {
    return e != NULL && e->type == bra && *e->begin == '{';
}

/*
Returns the next element that is not one of {whi, lbr, ind}; or NULL if there
is no such element.
*/
Element* skip_whi_lbr_ind(Element* e) {
    for (; e != NULL; e = e->next) {
        if (e->type != whi && e->type != lbr && e->type != ind) {
            return e;
        }
    }
    return e;
}

/*
Returns the next element that is not one of {whi, lbr, ind, sem}; or NULL if
there is no such element.
*/
Element* skip_whi_lbr_ind_sem(Element* e) {
    for (; e != NULL; e = e->next) {
        if (e->type != whi && e->type != lbr && e->type != sem && e->type != ind) {
            return e;
        }
    }
    return e;
}

/*
Returns the next element that is not one of {whi, lbr, ind, tok}; or NULL if
there is no such element.
*/
Element* skip_whi_lbr_ind_tok(Element* e) {
    for (; e != NULL; e = e->next) {
        if (e->type != whi && e->type != lbr && e->type != tok && e->type != ind) {
            return e;
        }
    }
    return e;
}

/*
Returns the next element that is not one of {whi, lbr, ind, lco, bco}; or NULL
if there is no such element.
*/
Element* skip_whi_lbr_ind_lco_bco(Element* e) {
    for (; e != NULL; e = e->next) {
        if (e->type != whi && e->type != lbr && e->type != ind 
                && e->type != lco && e->type != bco) {
            return e;
        }
    }
    return e;
}

static const char* PhraseStateNames[] = { 
    "s01", "s02", "s03", "s04", "s05", "s06", "s07", 
    "s08", "s09", "s10", "s11", "s12", "s13", "s14", 
    "s15", "s16", "s17", "s18", "s19", "PhraseStateCount"
};

static const char* PhraseTypeNames[] = {
    "error", "fun_dec", "fun_def", "var_dec", "var_def", "arr_dec", "arr_def", 
    "struct_or_union_def", "type_def", "preproc", "line_comment", "block_comment"
};

/*
Prints the phrase in the format [*PhraseType:<phrase contents>].followed by a
line break.
*/
void print_phrase(Phrase* phrase) {
    require_not_null(phrase);
    printf("[%s%s:", phrase->is_public ? "*" : "", PhraseTypeNames[phrase->type]);
    Element* e = phrase->first;
    Element* f = phrase->last;
    if (e != NULL && f != NULL) {
        char* p = e->begin;
        char* q = f->end;
        print_string(make_string2(p, q - p));
    }
    printf("]\n");
}

/*
State transition matrix for phrases. Each row represents a state. Each colum
represents an input.
*/
const PhraseState phrases[PhraseStateCount][8] = {
// tok,sem,b(),b{},b[],asg,pub,pre
  {s03,s14,s14,s14,s14,s14,s02,s19}, // s01: start
  {s03,s14,s14,s14,s14,s14,s02,s19}, // s02: pub
  {s03,s07,s04,s14,s10,s08,s14,s14}, // s03: pub? tok+
  {s14,s05,s14,s06,s14,s14,s14,s14}, // s04: pub? tok+ b()
  {s05,s05,s05,s05,s05,s05,s05,s05}, // s05: pub? tok+ b() sem -> fun_dec
  {s06,s06,s06,s06,s06,s06,s06,s06}, // s06: pub? tok+ b() b{} -> fun_def
  {s09,s09,s09,s09,s09,s09,s09,s09}, // s07: pub? tok+ sem -> var_dec
  {s08,s09,s08,s08,s08,s14,s08,s14}, // s08: pub? tok+ asg
  {s09,s09,s09,s09,s09,s09,s09,s09}, // s09: pub? tok+ asg !sem* sem -> var_def
  {s14,s11,s14,s14,s10,s12,s14,s14}, // s10: pub? tok+ b[]+
  {s11,s11,s11,s11,s11,s11,s11,s11}, // s11: pub? tok+ b[]+ sem -> arr_dec
  {s12,s13,s12,s12,s12,s14,s14,s14}, // s12: pub? tok+ b[]+ asg
  {s13,s13,s13,s13,s13,s13,s13,s13}, // s13: pub? tok+ b[]+ asg !sem* sem -> arr_def
  {s14,s14,s14,s14,s14,s14,s14,s14}, // s14: error
  {s20,s14,s14,s21,s14,s14,s14,s14}, // s15: pub? tok("struct"|"union")
  {s16,s16,s16,s16,s16,s16,s16,s16}, // s16: struct_or_union_def
  {s17,s18,s17,s17,s17,s17,s17,s14}, // s17: pub? tok("typedef")
  {s18,s18,s18,s18,s18,s18,s18,s18}, // s18: type_def
  {s19,s19,s19,s19,s19,s19,s19,s19}, // s19: pub? pre
  {s14,s16,s14,s21,s14,s14,s14,s14}, // s20: pub? tok("struct"|"union") tok
  {s14,s16,s14,s14,s14,s14,s14,s14}, // s21: pub? tok("struct"|"union") tok? b{}
};

State* next(State* s) {
    Element* e = s->input;
    require("valid input", e == NULL || (e->type != whi && e->type != lbr && e->type != ind));
    if (e == NULL) return s;
    e = e->next;
    e = skip_whi_lbr_ind(e);
    s->input = e;
    ensure("valid input", e == NULL || (e->type != whi && e->type != lbr && e->type != ind));
    return s;
}

ElementType symbol(State* s) {
    Element* e = s->input;
    if (e == NULL) return -1;
    return e->type;
}

void f_start000(State* state) {
    Element* e = state->input;
    // skip initial whitespace
    state->input = skip_whi_lbr_ind(state->input);
    f_start(state); 
    Phrase* p = &state->phrase;
    if (p->type == error) {
        switch (symbol(state)) {
            case pre: p->type = preproc; break;
            case lco: p->type = line_comment; break;
            case bco: p->type = block_comment; break;
            default: break;
        }
    }
    state->phrase.last = state->input;
}

// s01
// tok,sem,b(),b{},b[],asg,pub,pre
//{s03,s14,s14,s14,s14,s14,s02,s19}, // s01: start
void f_start(State* state) {
    switch (symbol(state)) {
        case tok: {
                Element* e = state->input;
                String s = make_string2(e->begin, e->end - e->begin);
                if (cstring_equal(s, "struct") || cstring_equal(s, "union") 
                        || cstring_equal(s, "enum")) {
                    f_struct_union_enum(next(state));
                } else if (cstring_equal(s, "typedef")) {
                    f_typedef(next(state));
                } else {
                    f_tok(next(state)); 
                }
            }
            break;
        case pub: f_pub(next(state)); break;
        case pre: f_pre(next(state)); break;
        default: f_err(state); break;
    }
}

// s02
// tok,sem,b(),b{},b[],asg,pub,pre
//{s03,s14,s14,s14,s14,s14,s02,s19}, // s02: pub
void f_pub(State* state) {
    state->phrase.is_public = true;
    switch (symbol(state)) {
        case tok: {
                Element* e = state->input;
                String s = make_string2(e->begin, e->end - e->begin);
                if (cstring_equal(s, "struct") || cstring_equal(s, "union") 
                        || cstring_equal(s, "enum")) {
                    f_struct_union_enum(next(state));
                } else if (cstring_equal(s, "typedef")) {
                    f_typedef(next(state));
                } else {
                    f_tok(next(state)); 
                }
            }
            break;
        case pre: f_pre(next(state)); break;
        default: f_err(state); break;
    }
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s03,s07,s04,s14,s10,s08,s14,s14}, // s03: pub? tok+
void f_tok(State* state) {
    Element* e = state->input;
    switch (symbol(state)) {
        case tok: f_tok(next(state)); break;
        case sem: f_tok_sem(state); break;
        case bra: 
            if (is_paren(e)) {
                f_tok_paren(next(state));
            } else if (is_bracket(e)) {
                f_tok_bracket(next(state));
            } else {
                f_err(state);
            }
            break;
        case asg: f_tok_asg(next(state)); break;
        default: f_err(state); break;
    }
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s14,s05,s14,s06,s14,s14,s14,s14}, // s04: pub? tok+ b()
void f_tok_paren(State* state) {
    Element* e = state->input;
    switch (symbol(state)) {
        case sem: f_tok_paren_sem(state); break;
        case bra: 
            if (is_curly(e)) {
                f_tok_paren_curly(state);
            } else {
                f_err(state);
            }
            break;
        default: f_err(state); break;
    }
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s05,s05,s05,s05,s05,s05,s05,s05}, // s05: pub? tok+ b() sem -> fun_dec
void f_tok_paren_sem(State* state) {
    state->phrase.type = fun_dec;
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s06,s06,s06,s06,s06,s06,s06,s06}, // s06: pub? tok+ b() b{} -> fun_def
void f_tok_paren_curly(State* state) {
    state->phrase.type = fun_def;
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s09,s09,s09,s09,s09,s09,s09,s09}, // s07: pub? tok+ sem -> var_dec
void f_tok_sem(State* state) {
    state->phrase.type = var_dec;
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s08,s09,s08,s08,s08,s14,s08,s14}, // s08: pub? tok+ asg
void f_tok_asg(State* state) {
    switch (symbol(state)) {
        case sem: f_tok_asg_sem(state); break;
        case asg: case pre: f_err(state); break;
        default: f_tok_asg(next(state)); break;
    }
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s09,s09,s09,s09,s09,s09,s09,s09}, // s09: pub? tok+ asg !sem* sem -> var_def
void f_tok_asg_sem(State* state) {
    state->phrase.type = var_def;
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s14,s11,s14,s14,s10,s12,s14,s14}, // s10: pub? tok+ b[]+
void f_tok_bracket(State* state) {
    switch (symbol(state)) {
        case sem: f_tok_bracket_sem(state); break;
        case bra: 
            if (is_bracket(state->input)) {
                f_tok_bracket(next(state));
            } else {
                f_err(state);
            }
            break;
        case asg: f_tok_bracket_asg(next(state)); break;
        default: f_err(state); break;
    }
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s11,s11,s11,s11,s11,s11,s11,s11}, // s11: pub? tok+ b[]+ sem -> arr_dec
void f_tok_bracket_sem(State* state) {
    state->phrase.type = arr_dec;
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s12,s13,s12,s12,s12,s14,s14,s14}, // s12: pub? tok+ b[]+ asg
void f_tok_bracket_asg(State* state) {
    switch (symbol(state)) {
        case sem: f_tok_bracket_asg_sem(state); break;
        case asg: case pub: case pre: f_err(state); break;
        default: f_tok_bracket_asg(state); break;
    }
}

//{s13,s13,s13,s13,s13,s13,s13,s13}, // s13: pub? tok+ b[]+ asg !sem* sem -> arr_def
void f_tok_bracket_asg_sem(State* state) {
    state->phrase.type = arr_def;
}

//{s14,s14,s14,s14,s14,s14,s14,s14}, // s14: error
void f_err(State* state) {
    state->phrase.type = arr_def;
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s20,s14,s14,s21,s14,s14,s14,s14}, // s15: pub? tok("struct"|"union")
void f_struct_union_enum(State* state) {
    switch (symbol(state)) {
        case tok: f_struct_union_enum_tok(next(state)); break;
        case sem: f_struct_union_enum_sem(state); break;
        default: f_err(state); break;
    }
}

//{s16,s16,s16,s16,s16,s16,s16,s16}, // s16: struct_or_union_def
void f_struct_union_enum_sem(State* state) {
    state->phrase.type = struct_or_union_def;
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s17,s18,s17,s17,s17,s17,s17,s14}, // s17: pub? tok("typedef")
void f_typedef(State* state) {
    switch (symbol(state)) {
        case sem: f_typedef_sem(state); break;
        case pre: f_err(state); break;
        default: f_typedef(next(state)); break;
    }
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s18,s18,s18,s18,s18,s18,s18,s18}, // s18: type_def
void f_typedef_sem(State* state) {
    state->phrase.type = type_def;
}

//{s19,s19,s19,s19,s19,s19,s19,s19}, // s19: pub? pre
void f_pre(State* state) {
    state->phrase.type = preproc;
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s14,s16,s14,s21,s14,s14,s14,s14}, // s20: pub? tok("struct"|"union") tok
void f_struct_union_enum_tok(State* state) {
    switch (symbol(state)) {
        case sem: f_struct_union_enum_sem(state); break;
        case bra: 
            if (is_curly(state->input)) {
                f_struct_union_enum_curly(next(state));
            } else {
                f_err(state);
            }
            break;
        default: f_err(state); break;
    }
}

// tok,sem,b(),b{},b[],asg,pub,pre
//{s14,s16,s14,s14,s14,s14,s14,s14}, // s21: pub? tok("struct"|"union") tok? b{}
void f_struct_union_enum_curly(State* state) {
    switch (symbol(state)) {
        case sem: f_struct_union_enum_sem(state); break;
        default: f_err(state); break;
    }
}

Phrase get_phrase(Element* list) {
    require_not_null(list);
    State state = (State){list, (Phrase){unknown, false, list, list}};
    // skip initial whitespace
    state.input = skip_whi_lbr_ind(state.input);
    f_start(&state); 
    Phrase* p = &state.phrase;
    if (p->type == error) {
        switch (symbol(&state)) {
            case lco: p->type = line_comment; break;
            case bco: p->type = block_comment; break;
            default: break;
        }
    }
    state.phrase.last = state.input;
    return state.phrase;
}



/*
Returns the next phrase starting at the given element.
*/
Phrase get_phrase0(Element* list) {
    require_not_null(list);
    Element* e = list;
    if (e->type == pre) return (Phrase){preproc, false, e, e};
    if (e->type == lco) return (Phrase){line_comment, false, e, e};
    if (e->type == bco) return (Phrase){block_comment, false, e, e};
    PhraseState state = s01;
    PhraseState prev_state = s01;
    bool public = false;
    while (true) {
        e = skip_whi_lbr_ind(e);
        if (e == NULL) break;
        int input = 0;
        // element types: ind, tok, whi, pre, lco, bco, sem, lbr, bra, asg, pub
        // element types: tok, pre, sem, bra, asg, pub
        // printf("%s: ", ElementTypeName[e->type]);
        // String ec = make_string2(e->begin, e->end - e->begin);
        // println_string(ec);
        switch (e->type) {
            case tok: input = 0; break;
            case sem: input = 1; break;
            case bra: input = is_paren(e) ? 2 : (is_curly(e) ? 3 : 4); break;
            case asg: input = 5; break;
            case pub: input = 6; break;
            case pre: input = 7; break;
            default: input = -1; break;
        }
        // printf("input = %d\n", input);
        if (input < 0) {
            // printf("%d, %s\n", input, ElementTypeName[e->type]);
            if (e->type == lco) {
                return (Phrase){line_comment, public, list, e};
            } else if (e->type == bco) {
                return (Phrase){block_comment, public, list, e};
            }
            return (Phrase){error, public, e, e};
        }
        assert("valid input", 0 <= input && input < 8);
        prev_state = state;
        state = phrases[state][input];
        assert("valid state", 0 <= state && state < PhraseStateCount);
        switch (state) {
            case s02: public = true; break;
            case s03: if (prev_state != s03) {
                    String s = make_string2(e->begin, e->end - e->begin);
                    if (cstring_equal(s, "struct") || cstring_equal(s, "union") || cstring_equal(s, "enum")) {
                        state = s15;
                    } else if (cstring_equal(s, "typedef")) {
                        state = s17;
                    }
                }
                break;
            case s05: return (Phrase){fun_dec, public, list, e};
            case s06: return (Phrase){fun_def, public, list, e};
            case s07: return (Phrase){var_dec, public, list, e};
            case s09: return (Phrase){var_def, public, list, e};
            case s11: return (Phrase){arr_dec, public, list, e};
            case s13: return (Phrase){arr_def, public, list, e};
            case s16: return (Phrase){struct_or_union_def, public, list, e};
            case s18: return (Phrase){type_def, public, list, e};
            case s19: return (Phrase){preproc, public, list, e};
            default: break;
        }
        e = e->next;
    }
    return (Phrase){error, public, e, e};
}

void print_phrases(Element* list) {
    require_not_null(list);
    Element* e = list;
    while (e != NULL) {
        e = skip_whi_lbr_ind_sem(e);
        if (e == NULL) break;
        Phrase phrase = get_phrase(e);
        // printf("phrase = %s\n", PhraseTypeNames[phrase.type]);
        print_phrase(&phrase);
        e = phrase.last;
        if (e == NULL) break;
        e = e->next;
    }
}

/*
Starting from begin, appends the contents of all elements until stop returns
true. Does not append the contents of the element for which stop returns true.
*/
void xappend_string_until(String* str, Element* first, bool stop(Element*)) {
    if (first == NULL || stop(first)) return;
    Element* last = first;
    for (Element* e = first; e != NULL && !stop(e); e = e->next) {
        if (e->type != whi) {
            last = e;
        }
    }
    xappend_cstring2(str, first->begin, last->end);
}

/*
Creates header file contents for the given list of elements.
*/
String create_header(/*in*/String basename, /*in*/Element* list) {
    require_not_null(list);
    String head = new_string(1024);
    xappend_cstring(&head, "#ifndef ");
    xappend_string(&head, basename);
    xappend_cstring(&head, "_h_INCLUDED\n#define ");
    xappend_string(&head, basename);
    xappend_cstring(&head, "_h_INCLUDED\n");
    Element* e = list;
    while (e != NULL) {
        e = skip_whi_lbr_ind_sem(e);
        if (e == NULL) break;
        Phrase phrase = get_phrase(e);
        if (DEBUG) printf("phrase = %s\n", PhraseTypeNames[phrase.type]);
        if (DEBUG) xappend_cstring(&head, "phrase = ");
        if (DEBUG) xappend_cstring(&head, (char*)PhraseTypeNames[phrase.type]);
        if (DEBUG) xappend_char(&head, '\n');
        if (phrase.type == error) {
            if (phrase.last != NULL) e = phrase.last;
            int line = count_lines(list->begin, e->end) + 1;
            fprintf(stderr, "%s:%d: Error\n", basename.s, line);
            exit(EXIT_FAILURE);
        }
        if (phrase.is_public) {
            Element* first = phrase.first->next; // skip pub
            Element* last = phrase.last;
            if (DEBUG) xappend_cstring2(&head, first->begin, last->end);
            if (DEBUG) xappend_char(&head, '\n');
            switch (phrase.type) {
                case var_dec:
                case arr_dec:
                    xappend_cstring(&head, "extern ");
                    xappend_cstring2(&head, first->begin, last->end);
                    xappend_char(&head, '\n');
                    break;
                case fun_dec:
                case preproc:
                    xappend_cstring2(&head, first->begin, last->end);
                    xappend_char(&head, '\n');
                    break;
                case fun_def:
                    xappend_string_until(&head, first, is_curly);
                    xappend_cstring(&head, ";\n");
                    break;
                case var_def:
                case arr_def:
                    xappend_cstring(&head, "extern ");
                    xappend_string_until(&head, first, is_asg);
                    xappend_cstring(&head, ";\n");
                    break;
                case struct_or_union_def:
                case type_def:
                    xappend_cstring2(&head, first->begin, last->end);
                    xappend_char(&head, '\n');
                    break;
                case line_comment:
                case block_comment:
                    xappend_cstring2(&head, first->begin, last->end);
                    xappend_char(&head, '\n');
                    break;
                default:
                    xappend_cstring(&head, "// phrase ");
                    xappend_cstring(&head, (char*)PhraseTypeNames[phrase.type]);
                    xappend_cstring(&head, " NOT HANDLED\n");
                    break;
            }
        }
        //print_phrase(&phrase);
        e = phrase.last;
        if (e == NULL) break;
        e = e->next;
    }
    xappend_cstring(&head, "#endif\n");
    return head;
}

String fun_name(Phrase phrase) {
    if (phrase.type == fun_def || phrase.type == fun_dec) {
        // last token in phrase is function name
        Element* token = NULL;
        for (Element* e = phrase.first; e != NULL && e != phrase.last; e = e->next) {
            if (e->type == tok) {
                token = e;
            }
        };
        if (token != NULL) {
            return make_string2(token->begin, token->end - token->begin);
        }
    }
    return make_string("");
}
/*
Creates implementation file contents for the given list of elements. Maintain
the line numbers line numbers of the original contents.
*/
String create_impl(/*in*/String basename, /*in*/Element* list) {
    require_not_null(list);
    int lines;
    String impl = new_string(1024);
    Element* e = list;
    while (e != NULL) {
        Element* f = skip_whi_lbr_ind_sem(e);
        if (f == NULL) {
            xappend_cstring(&impl, e->begin);
            break;
        } else {
            xappend_cstring2(&impl, e->begin, f->begin);
            e = f;
        }
        Phrase phrase = get_phrase(e);
        if (DEBUG) printf("phrase = %s\n", PhraseTypeNames[phrase.type]);
        if (DEBUG) xappend_cstring(&impl, "phrase = ");
        if (DEBUG) xappend_cstring(&impl, (char*)PhraseTypeNames[phrase.type]);
        if (DEBUG) xappend_char(&impl, '\n');
        if (phrase.type == error) {
            if (phrase.last != NULL) e = phrase.last;
            int line = count_lines(list->begin, e->end) + 1;
            fprintf(stderr, "%s:%d: Error\n", basename.s, line);
            exit(EXIT_FAILURE);
        }
        if (phrase.is_public) {
            Element* first = phrase.first->next; // skip pub
            Element* last = phrase.last;
            if (DEBUG) xappend_cstring2(&impl, first->begin, last->end);
            switch (phrase.type) {
                case var_dec:
                case var_def:
                case fun_dec:
                case fun_def:
                case arr_dec:
                case arr_def:
                    xappend_cstring2(&impl, first->begin, last->end);
                    break;
                case struct_or_union_def:
                case type_def:
                case preproc:
                    lines = count_lines(first->begin, last->end);
                    for (int i = 0; i < lines; i++) xappend_char(&impl, '\n');
                    break;
                default:
                    xappend_cstring2(&impl, first->begin, last->end);
                    break;
            }
        } else { // not public
            Element* first = phrase.first;
            Element* last = phrase.last;
            if (DEBUG) xappend_cstring2(&impl, first->begin, last->end);
            if (DEBUG) xappend_char(&impl, '\n');
            switch (phrase.type) {
                case fun_dec:
                case fun_def:
                    // do not put "static" in front of the main function
                    if (!cstring_equal(fun_name(phrase), "main")) {
                        xappend_cstring(&impl, "static ");
                    }
                    xappend_cstring2(&impl, first->begin, last->end);
                    break;
                case var_dec:
                case var_def:
                case arr_dec:
                case arr_def:
                    xappend_cstring(&impl, "static ");
                    xappend_cstring2(&impl, first->begin, last->end);
                    break;
                case struct_or_union_def:
                case type_def:
                case preproc:
                    xappend_cstring2(&impl, first->begin, last->end);
                    break;
                default:
                    xappend_cstring2(&impl, first->begin, last->end);
                    break;
            }
        }
        //print_phrase(&phrase);
        e = phrase.last;
        if (e == NULL) break;
        e = e->next;
    }
    return impl;
}
#endif

int main(int argc, char* argv[]) {
    // split_test();
    // split_lines_test();
    // indentation_test();
    // next_state_test();
    // trim_test();
    // trim_left_test();
    // trim_right_test();
    // index_of_test();
    // append_test();
    // xappend_test();
    // scan_next_test();
    // exit(0);

    if (argc != 2) {
        printf("Usage: headify <filename C file>\n");
        exit(EXIT_FAILURE);
    }

    // separate dirname and basename (without extension) from filename
    String filename = make_string(argv[1]);
    int idir = last_index_of_char(filename, '/') + 1;
    int iext = last_index_of_char(filename, '.');
    if (iext < 0) iext = filename.len;
    String dirname = make_string2(filename.s, idir);
    String basename = make_string2(filename.s + idir, iext - idir);
    bool ends_with_hy = ends_with(basename, make_string(".hy"));
    if (ends_with_hy) basename.len -= 3;
    if (basename.len <= 0) {
        printf("Usage: headify <filename C file>\n");
        exit(EXIT_FAILURE);
    }
    if (DEBUG) printf("%.*s, %.*s, %.*s\n", 
            filename.len, filename.s, 
            dirname.len, dirname.s, 
            basename.len, basename.s); 

    String source_code = read_file(filename.s);
    ElementList elements = get_elements(filename.s, source_code);
    //if (DEBUG) 
    print_elements(&elements);
    exit(EXIT_FAILURE);

#if 0
    Phrase phrase = get_phrase(elements.first);
    printf("phrase = %s\n", PhraseTypeNames[phrase.type]);
    print_phrase(&phrase);
#endif

#if 0
    Element* e = elements.first;
    while (e != NULL) {
        e = skip_whi_lbr_ind_sem(e);
        if (e == NULL) break;
        Phrase phrase = get_phrase(e);
        // printf("phrase = %s\n", PhraseTypeNames[phrase.type]);
        print_phrase(&phrase);
        e = phrase.last;
        if (e == NULL) break;
        e = e->next;
    }
#endif
#if 0
    if (DEBUG) print_phrases(elements.first);

    String head = create_header(basename, elements.first);
    String headname = new_string(256);
    xappend_string(&headname, dirname);
    xappend_string(&headname, basename);
    if (ends_with_hy) {
        xappend_cstring(&headname, ".h");
    } else {
        xappend_cstring(&headname, "_headify.h");
    }
    xappend_char(&headname, '\0');
    write_file(headname.s, head);
    free(headname.s);
    free(head.s);

    String impl = create_impl(basename, elements.first);
    String implname = new_string(256);
    xappend_string(&implname, dirname);
    xappend_string(&implname, basename);
    if (ends_with_hy) {
        xappend_cstring(&implname, ".c");
    } else {
        xappend_cstring(&implname, "_headify.c");
    }
    xappend_char(&implname, '\0');
    write_file(implname.s, impl);
    free(implname.s);
    free(impl.s);
#endif
    elements_free(&elements);
    free(source_code.s);
    return 0;
}
