/*
@author: Michael Rohs
@date: December 6, 2021
*/

#ifndef headify_h_INCLUDED
#define headify_h_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include "util.h"

/*
The source code string is parsed into Elements. The Elements are then grouped
into Phrases.

The possible Elements are: indentation, whitespace, token, string literal,
character literal, preprocessor directive, line comment, block comment,
semicolon, line break, brace (parentheses, brackets, or curly braces),
assignment character '=', or public signifier '*'.

The possible Phrases are: error, fun_dec, fun_def, var_dec, var_def, arr_dec,
arr_def, struct_or_union_def, type_def, preproc, line_comment, block_comment
*/
typedef enum ElementType ElementType;
enum ElementType { 
    ind, tok, whi, str, stx, 
    ste, chr, chx, che, pre, 
    lco, bco, bce, bci, bcf, 
    sem, lbr, bra, bre, asg, 
    pub, ElementTypeCount
};

typedef struct Element Element;
struct Element {
    ElementType type;
    char* begin;
    char* end; // exclusive
    Element* next;
};

/*
public = '*' in indentation_regin
indentation_region = block_comment* public? block_comment*
code = (ind? (preproc 
             |func_decl 
             |func_def 
             |var_decl 
             |var_def
             |arr_decl 
             |arr_def
             |line_comment
             |block_comment
             |struct_def
             |typedef)) *

element types: ind, tok, whi, pre, lco, bco, sem, lbr, bra, asg, pub

preproc = '#' chars line_break(unquoted)

func_decl = token+ parentheses_pair ';'
func_def  = token+ parentheses_pair curly_pair
var_decl = token+ ';'
var_def  = token+ '=' token ';'
arr_decl: token+, brackets+, semicolon
arr_def: token+, brackets+, assign, braces, semicolon

line_comment = '//' char line_break
block_comment = '/+' chars '+/'

struct_def = 'struct' chars ';'
union_def = 'union' chars ';'
type_def = 'typedef' chars ';'
*/

typedef enum PhraseState PhraseState;
enum PhraseState { 
    s01, s02, s03, s04, s05, s06, s07, 
    s08, s09, s10, s11, s12, s13, s14, 
    s15, s16, s17, s18, s19, s20, s21, 
    PhraseStateCount, 
};

typedef enum PhraseType PhraseType;
enum PhraseType {
    unknown, error, fun_dec, fun_def, var_dec, var_def, arr_dec, arr_def, 
    struct_or_union_def, type_def, preproc, line_comment, block_comment
};

typedef struct Phrase Phrase;
struct Phrase {
    PhraseType type;
    bool is_public; // is this a public phrase (to appear in the header file)?
    // first..last is a linked list of elements belonging to this phrase
    Element* first; // first element of phrase (inclusive)
    Element* last; // last element of phrase (inclusive)
};

typedef struct State State;
typedef State (*StateTransition)(State state);

struct State {
    Element* input;
    Phrase phrase;
};

void f_start(State* state);
void f_pub(State* state);
void f_tok(State* state);
void f_tok_paren(State* state);
void f_tok_paren_sem(State* state); // fun_dec	
void f_tok_paren_curly(State* state); // fun_def
void f_tok_sem(State* state); // var_dec
void f_tok_asg(State* state);
void f_tok_asg_sem(State* state); // var_def
void f_tok_bracket(State* state);
void f_tok_bracket_sem(State* state); // arr_dec
void f_tok_bracket_asg(State* state);
void f_tok_bracket_asg_sem(State* state); // arr_def
void f_struct_union_enum(State* state);
void f_struct_union_enum_tok(State* state);
void f_struct_union_enum_curly(State* state);
void f_struct_union_enum_sem(State* state); // struct_union_enum_def
void f_typedef(State* state);
void f_typedef_sem(State* state); // typedef_def
void f_pre(State* state); // preproc
void f_err(State* state); // error

#endif // headify_h_INCLUDED
