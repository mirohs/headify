/*
make vector && ./vector
*/;

#include <time.h>
#include "util.h"
#include "vector.h"

*typedef struct {
    int capacity;
    int count;
    int* data; }
Vector;

*Vector* vector_new(int capacity) {
    Vector* v = xcalloc(1, sizeof(Vector));
    v->capacity= capacity;
    v->data = xcalloc(v->capacity, sizeof(int));
    v->count = 0;
    return v; }

*void vector_free(Vector* v) {
    free(v->data);
    free(v); 
};

void vector_add(Vector* v, int x) {
    if(v->count >= v->capacity ) {
        v->capacity *= 2;
        fprintf(stderr, "reallocating, new capacity: %d ints\n", v->capacity);
        int* data_new = xcalloc(v->capacity, sizeof(int));
        memcpy(data_new, v->data, v->count * sizeof(int));
        free(v->data);
        v->data = data_new; 
    };
    v->data[v->count++] = x; 
};

*int vector_count(Vector* v) {
    return v->count; }

int vector_get(Vector* v, int i) {
    if(i < 0 || i >= v->count ) {
        fprintf(stderr, "error: index out of bounds\n");
        exit(1); 
    };
    return v->data[i]; }

*int main(void) {
    srand(time(NULL));
    Vector* v = vector_new(2);
    for(int i = 0; i < 100; i++ ) {
        vector_add(v, rand() % 100); }
    printf("count = %d\n", vector_count(v));
    for(int i = 0; i < 100; i++ ) {
        printf("%d, ", vector_get(v, i)); }
    printf("\n");
    
    vector_free(v);
    
    return 0; }
