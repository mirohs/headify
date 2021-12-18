#include "util.h"
#include "next_state.h"
;
static int states[8][8] = {
    
    
    { 1, 6, 0, 3, 4, 0, 5, 0 },
    { 0, 1, 2, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1 },
    { 3, 3, 3, 3, 3, 3, 3, 3 },
    { 4, 4, 4, 4, 4, 0, 4, 4 },
    { 5, 5, 5, 5, 5, 5, 5, 5 },
    { 6, 0, 7, 6, 6, 6, 6, 6 },
    { 6, 6, 6, 6, 6, 6, 6, 6 },
};
;
/*
Computes the next state given the current state and two subsequent characters
from the input.
*/;
int next_state(int state, char c, char d) {
    require("valid state", 0 <= state && state < 8);
    int input = 0;
    switch(c ) {
        case '"': input = 0; break;
        case '\'': input = 1; break;
        case '\\': {
            switch (d) {
                case '\n': case '\0': input = 6; break;
                default: input = 2; break; }
            break; }
        case '/': {
            switch (d) {
                case '/': input = 3; break;
                case '*': input = 4; break;
                default: input = 7; break; }
            break; }
        case '*': if (d == '/') input = 5; else input = 7; break;
        default: input = 7; break; 
    };
    ensure("valid input", 0 <= input && input < 8);
    state = states[state][input];
    ensure("valid state", 0 <= state && state < 8);
    return state; }
;
static void next_state_test(void) {
    test_equal_i(next_state(0, '"', 'y'), 1);
    test_equal_i(next_state(0, '\'', 'y'), 6);
    test_equal_i(next_state(0, '\\', 'y'), 0);
    test_equal_i(next_state(0, '/', '/'), 3);
    test_equal_i(next_state(0, '/', '*'), 4);
    test_equal_i(next_state(0, '*', '/'), 0);
    test_equal_i(next_state(0, 'x', 'y'), 0);
    test_equal_i(next_state(1, '"', 'y'), 0);
    test_equal_i(next_state(1, '\'', 'y'), 1);
    test_equal_i(next_state(6, '\'', 'y'), 0);
    test_equal_i(next_state(1, '\\', 'y'), 2);
    test_equal_i(next_state(1, '/', '/'), 1);
    test_equal_i(next_state(1, '/', '*'), 1);
    test_equal_i(next_state(1, '*', '/'), 1);
    test_equal_i(next_state(1, 'x', 'y'), 1);
    ;
    test_equal_i(next_state(0, '\\', '\0'), 5); }
;
int main(void) {
    next_state_test();
    return 0; }
