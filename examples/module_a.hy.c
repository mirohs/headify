#include <stdio.h>
#include <stdlib.h>
#include "module_a.h"

#define N 10
int a[N];

*int get_a(int i) {
    if (i < 0 || i >= N) {
        fprintf(stderr, "index out of bounds\n");
        exit(EXIT_FAILURE);
    }
    return a[i];
}

*void set_a(int i, int x) {
    if (i < 0 || i >= N) {
        fprintf(stderr, "index out of bounds\n");
        exit(EXIT_FAILURE);
    }
    a[i] = x;
}

int xmain(void) {
    set_a(3, 123);
    printf("%d\n", get_a(3));
    set_a(30, 123);
    return 0;
}
