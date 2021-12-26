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

The possible Elements are: error, whitespace, token (including string literal
and character literal), preprocessor directive, line comment, block comment,
semicolon, line break, pair of braces (parentheses, brackets, curly braces, or
any closing brace), assignment character '=', public signifier '*', or end of
source.

The possible Phrases are: error, fun_dec, fun_def, var_dec, var_def, arr_dec,
arr_def, struct_union_enum_def, type_def, preproc, line_comment, block_comment
*/
typedef enum ElementType ElementType;
enum ElementType { 
    err, whi, tok, pre, lco, bco, sem, 
    lbr, par, bra, cur, clo, asg, pub, 
    eos, ElementTypeCount
};

/*
whi = whichar {whi_char}.
whi_char = " " | "\t".
str = """ {str_char} """.
str_char = char | escaped_char.
chr = "'" chr_char "'".
chr_char = char | escaped_char.
pre = "#" {!lbr} /lbr.             lbr does not belong to pre
lbr = "\n".
lco = "//" {!lbr} /lbr.
bco = "/+" {char} "+/".
sem = ";".
asg = "=".
par = "(" exp ")".                 count number of enclosed braces
bra = "[" exp "]".
cur = "{" exp "}".
clo = ")" | "]" | "}".
pub = /lbr {/bco} "*".             lbr and bco do not belong to pub
        public signifier must appear in the indentation region of a line, 
        i.e. at the beginning of a line, optionally after whitespace and 
        block comments
eos = end_of_string.
tok = anything else, terminated by any of the first chars of the other elements
*/

typedef struct Element Element;
struct Element {
    ElementType type;
    char* begin; // inclusive
    char* end; // exclusive
    Element* next;
};

/*
public = '*' in indentation_regin
indentation_region = <line_start> block_comment* public? block_comment*
code = (ind? (preproc 
             |func_decl 
             |func_def 
             |var_decl 
             |var_def
             |arr_decl 
             |arr_def
             |line_comment
             |block_comment
             |struct_union_enum_def
             |typedef)) *

element types: whi, tok, pre, lco, bco, sem, lbr, par, bra, cur, asg, pub, 

preproc = '#' chars line_break(unquoted)

func_decl = token+ parentheses_pair ';'.
func_def  = token+ parentheses_pair curly_pair.
var_decl = token+ ';'.
var_def  = token+ '=' token ';'.
arr_decl = token+ brackets_pair+ ';'.
arr_def = token+ brackets_pair+ '=' curly_pair';'.

line_comment = '//' char line_break.
block_comment = '/+' chars '+/'.

struct_def = 'struct' chars ';'.
union_def = 'union' chars ';'.
union_def = 'enum' chars ';'.
type_def = 'typedef' chars ';'.
*/

typedef enum PhraseType PhraseType;
enum PhraseType {
    unknown, error, fun_dec, fun_def, var_dec, var_def, arr_dec, arr_def, 
    struct_union_enum_def, type_def, preproc, line_comment, block_comment
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

/*
source_code = {[pub] phrase}
phrase = fun_dec | fun_def | var_dec | var_def | arr_dec | arr_def 
       | struct_union_enum_def | typedef_def | preproc | error
lbr = "\n"
pub = lbr "*"
ind = {" " | "\t"}   after lbr
whi = {" " | "\t"}
var_dec = [pub] tok {tok} sem
var_def = [pub] tok {tok} asg {!sem} sem
tok = (letter | "_") {letter | digit | "_"}
*/

#endif // headify_h_INCLUDED
