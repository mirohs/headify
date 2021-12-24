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
    "ind", "tok", "whi", "str", "stx", 
    "ste", "chr", "chx", "che", "pre", 
    "lco", "bco", "bce", "bci", "bcf", 
    "sem", "lbr", "bra", "bre", "asg", 
    "pub", "ElementTypeCount"
};

// Generate the ElementList type and its associated functions. See util.h.
generate_list(ElementList, Element, elements_, );

/*
Creates a new element of the given type extending from begin (inclusive) to end
(exclusive). The next pointer is used to chain elements in a linked list.
*/
Element* new_element(ElementType type, char* begin, char* end, Element* next) {
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
    e->next = next;
    return e;
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
State transition matrix for elements. Each row represents a state. Each colum
represents an input. The input either consists of a single character or two
subsequent characters.
*/
const ElementType states[ElementTypeCount][15] = { // rows: states, columns: inputs
    //"   '   \   #   ;   \n  //  /*  */  ws  bo  bc   =   *  other
    //0   1   2   3   4   5   6   7   8   9   10  11  12  13  14
    {str,chr,ind,pre,sem,lbr,lco,bci,tok,ind,bra,bra,asg,pub,tok}, // ind indent
    {str,chr,tok,tok,sem,lbr,lco,bco,tok,whi,bra,bra,asg,tok,tok}, // tok token
    {str,chr,tok,tok,sem,lbr,lco,bco,tok,whi,bra,bra,asg,tok,tok}, // whi whitespace
    {ste,str,stx,str,str,str,str,str,str,str,str,str,str,str,str}, // str string
    {str,str,str,str,str,str,str,str,str,str,str,str,str,str,str}, // stx string_escape
    {str,chr,tok,tok,sem,lbr,lco,bco,tok,whi,bra,bra,asg,tok,tok}, // ste string_end
    {chr,che,chx,chr,chr,chr,chr,chr,chr,chr,chr,chr,chr,chr,chr}, // chr char
    {chr,chr,chr,chr,chr,chr,chr,chr,chr,chr,chr,chr,chr,chr,chr}, // chx char_escape
    {str,chr,tok,tok,sem,lbr,lco,bco,tok,whi,bra,bra,asg,tok,tok}, // che char_end
    {pre,pre,pre,pre,pre,lbr,pre,pre,pre,pre,pre,pre,pre,pre,pre}, // pre preproc
    {lco,lco,lco,lco,lco,lbr,lco,lco,lco,lco,lco,lco,lco,lco,lco}, // lco line_comment
    {bco,bco,bco,bco,bco,bco,bco,bco,bce,bco,bco,bco,bco,bco,bco}, // bco block_comment
    {str,chr,bce,tok,sem,lbr,lco,bco,tok,whi,bra,bra,asg,tok,tok}, // bce block_comment_end
    {bci,bci,bci,bci,bci,bci,bci,bci,bcf,bci,bci,bci,bci,bci,bci}, // bci block_comment_in_ind
    {str,chr,bcf,pre,sem,lbr,lco,bci,tok,ind,bra,bra,asg,pub,tok}, // bcf block_comment_in_ind_end
    {str,chr,sem,tok,sem,lbr,lco,bco,tok,whi,bra,bra,asg,tok,tok}, // sem semicolon
    {str,chr,lbr,pre,sem,lbr,lco,bci,pub,ind,bra,bra,asg,pub,tok}, // lbr line_break
    {bra,bra,bra,bra,bra,bra,bra,bra,bra,bra,bra,bra,bra,bra,bra}, // bra braces
    {str,chr,bre,tok,sem,lbr,lco,bco,tok,whi,bra,bra,asg,tok,tok}, // bre braces_end
    {str,chr,asg,tok,sem,lbr,lco,bco,tok,whi,bra,bra,asg,tok,tok}, // asg assign
    {str,chr,pub,pre,sem,lbr,lco,bco,tok,ind,bra,bra,asg,pub,tok}, // pub public
};

/*
Computes the next state given the current state and two subsequent characters
from the input. The state variable encodes the element type in the lower byte.
For the braces state, it encodes the brace level to be able to parse nested
braces.
*/
int next_state(int state, char c, char d) {
    assert("valid ElementTypeCount", sizeof(states) / sizeof(states[0]) == ElementTypeCount);
    int count = state >> 8;
    state &= 0xff;
    if (DEBUG) printf("next_state beg: %s, %d, %c%c\n", 
            ElementTypeName[state], count, c, d);
    require("valid state", 0 <= state && state < ElementTypeCount);
    assert("valid count for state", (state == bra && count > 0) || (state != bra && count == 0));
    int input = 0;
    switch (c) {
        case '"': input = 0; break;
        case '\'': input = 1; break;
        case '\\': input = 2; break;
        case '#': input = 3; break;
        case ';': input = 4; break;
        case '\n': input = 5; break;
        case '/': 
            switch (d) {
                case '/': input = 6; break;
                case '*': input = 7; break;
                default: input = 14; break;
            }
            break;
        case '*': if (d == '/') input = 8; else input = 13; break;
        case ' ': case '\t': input = 9; break;
        case '(': case '[': case '{': input = 10; break;
        case ')': case ']': case '}': input = 11; break;
        case '=': input = 12; break;
        default: input = 14; break;
    }
    assert("valid input", 0 <= input && input <= 14);
    state = states[state][input];
    assert("valid state", 0 <= state && state < ElementTypeCount);
    if (state == bra) {
        if (input == 10) {
            count++;
        } else if (input == 11) {
            count--;
            if (count == 0) {
                state = bre;
            }
        }
        state |= count << 8;
    }
    if (DEBUG) printf("next_state ret: %s, %d\n", 
            ElementTypeName[state & 0xff], state >> 8);
    return state;
}

/*
Test functino for next_state.
*/
void next_state_test(void) {
    test_equal_i(next_state(ind, '"', 'y'), str);
    test_equal_i(next_state(ind, '\'', 'y'), chr);
    test_equal_i(next_state(ind, '\\', 'y'), ind);
    test_equal_i(next_state(ind, '/', '/'), lco);
    test_equal_i(next_state(ind, '/', '*'), bci);
    test_equal_i(next_state(ind, '*', '/'), tok);
    test_equal_i(next_state(ind, 'x', 'y'), tok);
    test_equal_i(next_state(str, '"', 'y'), ste);
    test_equal_i(next_state(str, '\'', 'y'), str);
    test_equal_i(next_state(chr, '\'', 'y'), che);
    test_equal_i(next_state(str, '\\', 'y'), stx);
    test_equal_i(next_state(str, '/', '/'), str);
    test_equal_i(next_state(str, '/', '*'), str);
    test_equal_i(next_state(str, '*', '/'), str);
    test_equal_i(next_state(str, 'x', 'y'), str);
    test_equal_i(next_state(ind, '(', ' '), (1 << 8) | bra);
    test_equal_i(next_state((1 << 8) | bra, '(', ' '), (2 << 8) | bra);
    test_equal_i(next_state((2 << 8) | bra, ')', ' '), (1 << 8) | bra);
    test_equal_i(next_state((1 << 8) | bra, ')', ' '), bre);
    test_equal_i(next_state(bre, ' ', '/'), whi);
}

/*
Parses the source code string into a list of elements.
*/
ElementList get_elements(char* filename, String source_code) {
    require_not_null(filename);
    int state = ind, count = 0, prev_state = ind;
    int line_number = 1;
    ElementList elements = {NULL, NULL};
    char* begin = source_code.s;
    char* end = NULL;
    for (int i = 0; i < source_code.len; i++) {
        char c = source_code.s[i];
        char d = source_code.s[i + 1];
        // skip line continuation
        if (c == '\\' && d == '\n') {
            i++; // continue after newline
            continue;
        }
        prev_state = state;
        state = next_state(state | (count << 8), c, d);
        count = state >> 8;
        state = state & 0xff;
        if (DEBUG) printf("%4d: %c%c %s %s %d\n", i, c, d, ElementTypeName[prev_state], 
                ElementTypeName[state], count);
        if (c == '\n') line_number++;

        // some elements are only created when the state changes
        if (prev_state != state) {
            end = source_code.s + i;
            // create an element if it is complete now
            if (end > begin) {
                if (prev_state == ind || prev_state == tok || prev_state == whi 
                        || prev_state == pre || prev_state == lco) {
                    //if (tok == "struct")...
                    elements_append(&elements, new_element(prev_state, begin, end, NULL));
                    begin = end;
                }
            }

            if (state == str && prev_state != stx) {
                begin = source_code.s + i;
            } else if (state == ste) {
                end = source_code.s + i + 1;
                // treat string literals as token elements
                elements_append(&elements, new_element(tok, begin, end, NULL));
                begin = end;
            } else if (state == chr && prev_state != chx) {
                begin = source_code.s + i;
            } else if (state == che) {
                end = source_code.s + i + 1;
                // treat character literals as token elements
                elements_append(&elements, new_element(tok, begin, end, NULL));
                begin = end;
            } else if (state == pre || state == lco || state == bco || state == bci) {
                begin = source_code.s + i;
            } else if (state == bce || state == bcf) {
                end = source_code.s + i + 2;
                elements_append(&elements, new_element(bco, begin, end, NULL));
                i++;
                begin = end;
            } else if (state == bra && count == 1) {
                begin = source_code.s + i;
            } else if (state == bre) {
                end = source_code.s + i + 1;
                char bo = *begin;
                char bc = *(end - 1);
                if (!braces_match(bo, bc)) {
                    fprintf(stderr, "%s:%d: Opening and closing brace do not match.\n", 
                            filename, line_number);
                    exit(EXIT_FAILURE);
                }
                elements_append(&elements, new_element(bra, begin, end, NULL));
                begin = end;
            }
        } // if (prev_state != state)

        // some elements are created on every occurrence of the state
        if (state == sem || state == lbr || state == asg || state == pub) {
            end = begin + 1;
            elements_append(&elements, new_element(state, begin, end, NULL));
            begin = end;
        }
    } // for all chars of source_code
    return elements;
}

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
    if (DEBUG) print_elements(&elements);

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

    elements_free(&elements);
    free(source_code.s);
    return 0;
}
