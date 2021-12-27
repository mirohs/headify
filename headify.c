/*
@author: Michael Rohs
@date: December 6, 2021
*/

#include "util.h"
#include "headify.h"

const int DEBUG = false;

static const char* ElementTypeName[] = { 
    "err", "whi", "tok", "pre", "lco", "bco", "sem", 
    "lbr", "par", "bra", "cur", "clo", "asg", "pub", 
    "eos", "ElementTypeCount"
};

// Generate the ElementList type and its associated functions. See util.h.
generate_list(ElementList, Element, elements_, );

/*
Creates a new dynamically allocated element of the given type extending from
begin (inclusive) to end (exclusive).
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
(inclusive) to end (exclusive).
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
Prints the elements of the list.
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
int count_line_breaks(char* s, char* t) {
    require_not_null(s);
    require_not_null(t);
    int n = 0;
    for (; s < t; s++) {
        if (*s == '\n') n++;
    }
    return n;
}

// Is the scanner in the indentation region at the beginning of a line?
static bool indent = true;

// Contains the error message in case of an error.
static char* error_message = NULL;

// Contains the error position in case of an error.
static char* error_pos = NULL;

/*
Gets the next element from the source text.
*/
Element scan_next(char* s) {
    require_not_null(s);
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
        // indent state does not change for whitespace
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
            // indent state does not change for block comment
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
            indent = false; 
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
                    /* else '[' */ return make_element(bra, s, t);
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
        assert("not eos, at least one char in token", c != '\0');
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

    indent = true;
    e = scan_next("#a\\\nb\nc"); // line continuation
    test_equal_element(e, pre, "#a\\\nb", "\nc");

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

    e = scan_next(")abc");
    test_equal_element(e, clo, ")", "abc");

    e = scan_next("}abc");
    test_equal_element(e, clo, "}", "abc");

    e = scan_next("]abc");
    test_equal_element(e, clo, "]", "abc");

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
Parses the source text into a list of elements.
*/
ElementList get_elements(char* filename, char* source_code) {
    require_not_null(filename);
    require_not_null(source_code);
    ElementList elements = {NULL, NULL};
    Element e = scan_next(source_code);
    while (e.type != eos) {
        if (e.type == err) {
            int line = count_line_breaks(source_code, error_pos) + 1;
            fprintf(stderr, "%s:%d: %s\n", filename, line, error_message);
            exit(EXIT_FAILURE);
        }
        elements_append(&elements, new_element(e.type, e.begin, e.end));
        e = scan_next(e.end);
    }
    return elements;
}

// Checks if e is an assignment.
bool is_asg(Element* e) {
    return e != NULL && e->type == asg;
}

// Checks if e is a curly braces element {...}.
bool is_curly(Element* e) {
    return e != NULL && e->type == cur;
}

// Is the element a struct or union or enum token?
bool is_struct_union_enum(Element* e) {
    if (e == NULL || e->type != tok) return false;
    String s = make_string2(e->begin, e->end - e->begin);
    return cstring_equal(s, "struct") 
        || cstring_equal(s, "union") 
        || cstring_equal(s, "enum");
}

// Is the element a typedef token?
bool is_typedef(Element* e) {
    if (e == NULL || e->type != tok) return false;
    String s = make_string2(e->begin, e->end - e->begin);
    return cstring_equal(s, "typedef");
}

/*
Returns the next element that is neither whi nor lbr; or NULL if there is no
such element.
*/
Element* skip_whi_lbr(Element* e) {
    for (; e != NULL; e = e->next) {
        if (e->type != whi && e->type != lbr) {
            return e;
        }
    }
    return NULL;
}

/*
Returns the next element that is neither whi nor lbr nor sem; or NULL if there
is no such element.
*/
Element* skip_whi_lbr_sem(Element* e) {
    for (; e != NULL; e = e->next) {
        if (e->type != whi && e->type != lbr && e->type != sem) {
            return e;
        }
    }
    return NULL;
}

static const char* PhraseTypeNames[] = {
    "unknown", "error", "fun_dec", "fun_def", "var_dec", "var_def", "arr_dec", "arr_def", 
    "struct_union_enum_def", "type_def", "preproc", "line_comment", "block_comment"
};

/*
Prints the phrase in the format [*PhraseType:<phrase contents>]. followed by a
line break. The '*' indicates a public phrase.
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
Goes to next non-whitespace and non-linebreak element. Sets input to NULL if no
such element exists.
*/
State* next(State* s) {
    require_not_null(s);
    Element* e = s->input;
    require("valid input", e == NULL || (e->type != whi && e->type != lbr));
    if (e == NULL) return s;
    e = e->next;
    e = skip_whi_lbr(e);
    s->input = e;
    ensure("valid input", e == NULL || (e->type != whi && e->type != lbr));
    return s;
}

/*
Returns the current input symbol; or eos if the end of the input has been
reached.
*/
ElementType symbol(State* s) {
    require_not_null(s);
    Element* e = s->input;
    if (e == NULL) return eos;
    return e->type;
}

/*
The following functions, named f_..., detect a Phrase from sequence of Elements.
The functions implement a simple recusive descent parser.
*/

void f_start(State* state) {
    switch (symbol(state)) {
        case tok: {
                Element* e = state->input;
                if (is_struct_union_enum(e)) {
                    f_struct_union_enum(next(state));
                } else if (is_typedef(e)) {
                    f_typedef(next(state));
                } else {
                    f_tok(next(state)); 
                }
            }
            break;
        case pub: f_pub(next(state)); break;
        case pre: f_pre(state); break;
        case lco: f_lco(state); break;
        case bco: f_bco(state); break;
        default: f_err(state); break;
    }
}

void f_pub(State* state) {
    state->phrase.is_public = true;
    switch (symbol(state)) {
        case tok: {
                Element* e = state->input;
                if (is_struct_union_enum(e)) {
                    f_struct_union_enum(next(state));
                } else if (is_typedef(e)) {
                    f_typedef(next(state));
                } else {
                    f_tok(next(state)); 
                }
            }
            break;
        case pre: f_pre(state); break;
        case lco: f_lco(state); break;
        case bco: f_bco(state); break;
        default: f_err(state); break;
    }
}

void f_tok(State* state) {
    switch (symbol(state)) {
        case tok: f_tok(next(state)); break;
        case sem: f_tok_sem(state); break;
        case par: f_tok_paren(next(state)); break;
        case bra: f_tok_bracket(next(state)); break;
        case asg: f_tok_asg(next(state)); break;
        case lco: case bco: f_tok(next(state)); break;
        default: f_err(state); break;
    }
}

void f_tok_paren(State* state) {
    switch (symbol(state)) {
        case sem: f_tok_paren_sem(state); break;
        case cur: f_tok_paren_curly(state); break;
        case lco: case bco: f_tok_paren(next(state)); break;
        default: f_err(state); break;
    }
}

void f_tok_paren_sem(State* state) {
    state->phrase.type = fun_dec;
}

void f_tok_paren_curly(State* state) {
    state->phrase.type = fun_def;
}

void f_tok_sem(State* state) {
    state->phrase.type = var_dec;
}

void f_tok_asg(State* state) {
    switch (symbol(state)) {
        case sem: f_tok_asg_sem(state); break;
        default: f_tok_asg(next(state)); break;
    }
}

void f_tok_asg_sem(State* state) {
    state->phrase.type = var_def;
}

void f_tok_bracket(State* state) {
    switch (symbol(state)) {
        case sem: f_tok_bracket_sem(state); break;
        case bra: f_tok_bracket(next(state)); break;
        case asg: f_tok_bracket_asg(next(state)); break;
        case lco: case bco: f_tok_bracket(next(state)); break;
        default: f_err(state); break;
    }
}

void f_tok_bracket_sem(State* state) {
    state->phrase.type = arr_dec;
}

void f_tok_bracket_asg(State* state) {
    switch (symbol(state)) {
        case sem: f_tok_bracket_asg_sem(state); break;
        default: f_tok_bracket_asg(next(state)); break;
    }
}

void f_tok_bracket_asg_sem(State* state) {
    state->phrase.type = arr_def;
}

void f_struct_union_enum(State* state) {
    switch (symbol(state)) {
        case sem: f_struct_union_enum_sem(state); break;
        default: f_struct_union_enum(next(state)); break;
    }
}

void f_struct_union_enum_sem(State* state) {
    state->phrase.type = struct_union_enum_def;
}

void f_typedef(State* state) {
    switch (symbol(state)) {
        case sem: f_typedef_sem(state); break;
        default: f_typedef(next(state)); break;
    }
}

void f_typedef_sem(State* state) {
    state->phrase.type = type_def;
}

void f_pre(State* state) {
    state->phrase.type = preproc;
}

void f_lco(State* state) {
    state->phrase.type = line_comment;
}

void f_bco(State* state) {
    state->phrase.type = block_comment;
}

void f_err(State* state) {
    state->phrase.type = error;
}

/*
Returns the next phrase starting at the given element.
*/
Phrase get_phrase(Element* list) {
    require_not_null(list);
    State state = (State){list, (Phrase){unknown, false, list, list}};
    // skip initial whitespace
    state.input = skip_whi_lbr(state.input);
    f_start(&state); 
    state.phrase.last = state.input;
    return state.phrase;
}

#define test_phrase(source_code, type, public) \
    base_test_phrase(__FILE__, __LINE__, source_code, type, public)

bool base_test_phrase(char* file, int line, char* s, PhraseType type, bool public) {
    indent = true;
    printf("\n%s\n", s);
    ElementList elements = get_elements("", s);
    // print_elements(&elements);
    Phrase p = get_phrase(elements.first);
    print_phrase(&p);
    bool ok1 = base_test_equal_i(file, line, p.type, type);
    if (!ok1) printf("\t\ttypes: actual = %s, expected = %s\n", 
            PhraseTypeNames[p.type], PhraseTypeNames[type]);
    bool ok2 = base_test_equal_i(file, line, p.is_public, public);
    return ok1 && ok2;
}

void get_phrase_test(void) {
    test_phrase("int i;X", var_dec, false);
    test_phrase("*int i;X", var_dec, true);
    test_phrase("int i=123;X", var_def, false);
    test_phrase(" * int i=123;X", var_def, true);
    test_phrase("int i=123, j = 2;X", var_def, false);
    test_phrase("int i=123, *j;X", var_def, false);
    test_phrase("int/*block comment*/i;X", var_dec, false);
    test_phrase("int//comm \niii = 123;X", var_def, false);

    test_phrase("char * abc;X", var_dec, false);
    test_phrase("char * abc = \"abc\";X", var_def, false);
    test_phrase("*char*abc=\"abc\";X", var_def, true);

    test_phrase("int a[10];X", arr_dec, false);
    test_phrase("int a[] = {1, 2, 3};X", arr_def, false);
    test_phrase("*int a[2, 2] = {{1, 2}, {3, 4}};X", arr_def, true);

    test_phrase("int f(int i);X", fun_dec, false);
    test_phrase("int f(int i){return 2*i;}X", fun_def, false);
    test_phrase("\n\n\t int \nf(int i)\n{\nreturn \n2*i;\n}\nX", fun_def, false);
    test_phrase("static int * f(int* i, int (*g)(int,int)){return 2*i;}X", fun_def, false);

    test_phrase("   // line comment\nX", line_comment, false);
    test_phrase(" * // line comment\nX", line_comment, true);

    test_phrase("  /* block comment */X", block_comment, false);
    test_phrase("  */* block comment */X", block_comment, true);

    test_phrase("      #abc", preproc, false);
    test_phrase("  #abc\nX", preproc, false);
    test_phrase("  #abc\\\ndef\nX", preproc, false);
    test_phrase(" * #abc\\\ndef\nX", preproc, true);

    test_phrase("struct s;X", struct_union_enum_def, false);
    test_phrase("struct Point{int x; int y;};X", struct_union_enum_def, false);
    test_phrase("*struct Point{int x[10]; int y;};X", struct_union_enum_def, true);

    test_phrase("union s;X", struct_union_enum_def, false);
    test_phrase("union Point{int x; int y;};X", struct_union_enum_def, false);
    test_phrase("*union Point{int x[10]; int y;};X", struct_union_enum_def, true);

    test_phrase("enum s;X", struct_union_enum_def, false);
    test_phrase("*enum PhraseType{a, b=10, c, d};X", struct_union_enum_def, true);

    test_phrase("typedef void* Any;X", type_def, false);
    test_phrase("typedef char* (*f)(char*) MyFunc;X", type_def, false);
    test_phrase("*typedef struct {double x; double y;} Point;X", type_def, true);
    test_phrase("*typedef struct Point {double x; double y;} Point;X", type_def, true);
    test_phrase("*typedef char[ID_LEN] Ident;", type_def, true);
}

/*
Prints the list of phrases.
*/
void print_phrases(Element* list) {
    require_not_null(list);
    Element* e = list;
    while (e != NULL) {
        e = skip_whi_lbr_sem(e);
        if (e == NULL) break;
        Phrase phrase = get_phrase(e);
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
        e = skip_whi_lbr_sem(e);
        if (e == NULL) break;
        Phrase phrase = get_phrase(e);
        if (DEBUG) printf("phrase = %s\n", PhraseTypeNames[phrase.type]);
        if (DEBUG) xappend_cstring(&head, "phrase = ");
        if (DEBUG) xappend_cstring(&head, (char*)PhraseTypeNames[phrase.type]);
        if (DEBUG) xappend_char(&head, '\n');
        if (phrase.type == error) {
            if (phrase.last != NULL) e = phrase.last;
            int line = count_line_breaks(list->begin, e->end) + 1;
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
                case struct_union_enum_def:
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

/*
If phrase is a function definition or a function declaration, returns the
function name. Otherwise returns the empty string.
*/
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
Creates implementation file contents for the given list of elements. Maintains
the line numbers line numbers of the original contents.
*/
String create_impl(/*in*/String basename, /*in*/Element* list) {
    require_not_null(list);
    int lines;
    String impl = new_string(1024);
    Element* e = list;
    while (e != NULL) {
        Element* f = skip_whi_lbr_sem(e);
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
            int line = count_line_breaks(list->begin, e->end) + 1;
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
                case struct_union_enum_def:
                case type_def:
                case preproc:
                    lines = count_line_breaks(first->begin, last->end);
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
                case struct_union_enum_def:
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
    // scan_next_test();
    // get_phrase_test();
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
    ElementList elements = get_elements(filename.s, source_code.s);
    if (DEBUG) print_elements(&elements);

#if 0
    Phrase phrase = get_phrase(elements.first);
    printf("phrase = %s\n", PhraseTypeNames[phrase.type]);
    print_phrase(&phrase);
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
