/*
The transformation that `headify`performs, depend on the type of the entity and
whether it is marked as public or not. The his example shows each of the
possible entities.
*/

#include <stdio.h>
#include "transformations.h"

// Entity            | Public |     Header File     | Implementation File
// function definition | yes | function declaration | function definition
*int function_definition_public(int a, int b) {
    return a + b;
}

// Entity            | Public | Header File | Implementation File
// function definition | no | -             | `static` function definition
int function_definition_private(int a, int b) {
    return a + b;
}

// Entity             | Public |     Header File | Implementation File
// function declaration | yes | function declaration | function declaration
*int function_declaration_public(int a, int b);

// Entity             | Public | Header File | Implementation File
// function declaration | no | -             | `static` function declaration
int function_declaration_private(int a, int b);

// Entity             | Public |        Header File          | Implementation File
// variable definition | yes | `extern` variable declaration | variable definition
*int variable_definition_public = 123;
*int variable_definition2_public[2][3] = {
    {1, 2, 3}, 
    {4, 5, 6}
};

// Entity            | Public | Header File | Implementation File
// variable definition | no |       -       | `static` variable definition
int variable_definition_private = 456;
int variable_definition2_private[2][3] = {
    {1, 2, 3}, 
    {4, 5, 6}
};

// Entity             | Public |             Header File      | Implementation File
// variable declaration | yes | `extern` variable declaration | variable declaration
*int variable_declaration_public;
*int* variable_declaration2_public;
*int variable_declaration3_public[2][3];

// Entity            | Public | Header File | Implementation File
// variable declaration | no |      -       | `static` variable declaration
int variable_declaration_private;
int* variable_declaration2_private;
int variable_declaration3_private[2][3];

// Entity         | Public | Header File     | Implementation File
// struct or union | yes   | struct or union | -
*struct StructPublic {
    int x; 
    int y;
};
*union UnionPublic {
    int x; 
    float f;
};

// Entity        | Public | Header File | Implementation File
// struct or union | no |       -       | struct or union
struct StructPrivate {
    int x; 
    int y;
};
union UnionPrivate {
    int x; 
    float f;
};

// Entity         | Public | Header File     | Implementation File
// type definition | yes   | type definition | -
*typedef struct StructPublic TypedefPublic;
*typedef int*(*MyFuncPublic)(int, double);

// Entity        | Public | Header File | Implementation File
// type definition | no   | -           | type definition
typedef struct StructPrivate TypedefPrivate;
typedef int*(*MyFuncPrivate)(int, double);

// Entity              | Public | Header File            | Implementation File
// preprocessor directive | yes | preprocessor directive | -
*#define PREPROC_PUBLIC 12
*#define ABC_PUBLIC \
        34

// Entity               | Public | Header File | Implementation File
// preprocessor directive | no   |        -    | preprocessor directive
#define PREPROC_PRIVATE 56
#define ABC_PRIVATE \
        78

// Entity     | Public | Header File | Implementation File
// line comment | yes | line comment | line comment
*// Public line comment.

// Entity     | Public | Header File | Implementation File
// line comment | no   |      -      | line comment
// Private line comment.

// Entity      | Public | Header File   | Implementation File
// block comment | yes  | block comment | block comment
*/*
Public Block comment.
*/

// Entity      | Public | Header File | Implementation File
// block comment | no   |     -       | block comment
/*
Private Block comment.
*/

int main(void) {
    printf("%d\n", function_definition_public(variable_definition_private, PREPROC_PRIVATE));
    printf("%d\n", ABC_PUBLIC);
    printf("%d\n", ABC_PRIVATE);
    return 0;
}
