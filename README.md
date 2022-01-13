# headify

## Automatically extract header files from C files.

C requires maintaining both an implementation (.c) file and a header (.h) file. Manually keeping them consistent is tedious. *Headify* is a tool that automatically extracts header files from C files. 

To support automatic extraction, a *public* specifier in the form of a single `*` character is introduced. It is only parsed as a *public* specifier if it appears before any other tokens at the beginning of a line. Any entity that is not explicitly marked as *public* will be treated as a private entity of the module. This allows for simple information hiding in modules (translation units).

## Examples

The following examples illustrate the idea.

### module\_a.h.c

The file `module_a.h.c` has the private integer array `a`and two public functions `get_a` and `set_a`. Note that both functions are marked with a `*` at the beginning of the line. Only the public functions will appear in the header file (the "interface") of the module.

```c
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

int main(void) {
    set_a(3, 123);
    printf("%d\n", get_a(3));
    set_a(30, 123);
    return 0;
}
```

### module\_a.h

The command `headify module_a.h.c` generates the header file `module_a.h` and the implementation file `module_a.c`. These can then be compiled. Only the public entities appear in the header file.

```c
#ifndef module_a_h_INCLUDED
#define module_a_h_INCLUDED
int get_a(int i);
void set_a(int i, int x);
#endif
```

### module\_a.c

In the implementation file, entities that are not explicitly marked as public (`*`) are prefixed with the keyword `static`, which means that these entities are private to the translation unit and cannot be accessed from other translation units. The `main` function is treated specially, because it should never be prefixed with `static`.

```c
#include <stdio.h>
#include <stdlib.h>
#include "module_a.h"

#define N 10
static int a[N];

int get_a(int i) {
    if (i < 0 || i >= N) {
        fprintf(stderr, "index out of bounds\n");
        exit(EXIT_FAILURE);
    }
    return a[i];
}

void set_a(int i, int x) {
    if (i < 0 || i >= N) {
        fprintf(stderr, "index out of bounds\n");
        exit(EXIT_FAILURE);
    }
    a[i] = x;
}

int main(void) {
    set_a(3, 123);
    printf("%d\n", get_a(3));
    set_a(30, 123);
    return 0;
}
```

### module\_b.h.c

Let's have a look at a second module, which uses the first one. 

```c
#include <stdio.h>
#include "module_a.h"
#include "module_b.h"

*typedef struct Pair Pair;
struct Pair {
    int x;
    int y;
};

*Pair make_pair(int x, int y) {
    return (Pair) { x, y };
}

*int pair_x(Pair p) {
    return p.x;
}

*int pair_y(Pair p) {
    return p.y;
}

int main(void) {
    set_a(3, 123);
    printf("%d\n", get_a(3));
    Pair p = make_pair(10, 20);
    printf("(%d, %d)\n", pair_x(p), pair_y(p));
    return 0;
}
```

The type definition is marked as public, but not the structure itself. Hence, the structure is an opaque entity in the interface, which is beneficial for information hiding. The constructor function `make_pair`and the two accessor functions `pair_x` and `pair_y` are also marked as public.

The command `headify module_b.h.c` will generate `module_b.h` and  `module_b.c`. 


### module\_b.h

```c
#ifndef module_b_h_INCLUDED
#define module_b_h_INCLUDED
typedef struct Pair Pair;
Pair make_pair(int x, int y);
int pair_x(Pair p);
int pair_y(Pair p);
#endif
```

### module\_b.c

In the implementation file, the type definition no longer appears, because it has moved to the header file. The line numbers are preserved (by keeping the line breaks after after any removed entities), so that line numbers in compiler error messages are meaningful in the original `.c` file. 

```c
#include <stdio.h>
#include "module_a.h"
#include "module_b.h"


struct Pair {
    int x;
    int y;
};

Pair make_pair(int x, int y) {
    return (Pair) { x, y };
}

int pair_x(Pair p) {
    return p.x;
}

int pair_y(Pair p) {
    return p.y;
}

int main(void) {
    set_a(3, 123);
    printf("%d\n", get_a(3));
    Pair p = make_pair(10, 20);
    printf("(%d, %d)\n", pair_x(p), pair_y(p));
    return 0;
}
```

## Transformations

The transformation that *headify* performs, depend on the type of the entity and whether it is marked as public or not. The table shows each of the possible entities.

| Entity | Public | Header File | Implementation File |
| --- | --- | --- | --- | 
| function definition | yes | function declaration | function definition |
| function definition | no | - | `static` function definition |
| function declaration | yes | function declaration | function declaration |
| function declaration | no | - | `static` function declaration |
| variable definition | yes | `extern` variable declaration | variable definition |
| variable definition | no | - | `static` variable definition |
| variable declaration | yes | `extern` variable declaration | variable declaration |
| variable declaration | no | - | `static` variable declaration |
| struct or union | yes | struct or union | - |
| struct or union | no | - | struct or union |
| enum | yes | enum | - |
| enum | no | - | enum |
| type definition | yes | type definition | - |
| type definition | no | - | type definition |
| preprocessor directive | yes | preprocessor directive | - |
| preprocessor directive | no | - | preprocessor directive |
| line comment | yes | line comment | line comment |
| line comment | no | - | line comment |
| block comment | yes | block comment | block comment |
| block comment | no | - | block comment |

