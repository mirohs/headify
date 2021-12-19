#include <stdio.h>
#include "module_a_headify.h"
#include "module_b_headify.h"

*typedef struct Pair Pair;
struct Pair {
    int x;
    int y;
};

*Pair make_pair(int x, int y) {
    return (Pair) { x, y };
}

*// My line comment.
*int pair_x(Pair p) {
    return p.x;
}

*int pair_y(Pair p) {
    return p.y;
}

*/*
Myblock comment.
*/
int main(void) {
    set_a(3, 123);
    printf("%d\n", get_a(3));
    Pair p = make_pair(10, 20);
    printf("(%d, %d)\n", pair_x(p), pair_y(p));
    return 0;
}
