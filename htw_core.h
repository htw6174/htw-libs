#ifndef HTW_CORE_H_INCLUDED
#define HTW_CORE_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>

/* Debug utilities */
#define HTW_STOPWATCH(x) { clock_t start = clock(); \
                    x; \
                    clock_t end = clock(); \
                    float seconds = (float)(end - start) / CLOCKS_PER_SEC; \
                    printf("%s finished in %.3f seconds / %li ticks\n", #x, seconds, end - start); }

static inline void htw_printArray(FILE* dest, void* data, unsigned int size, unsigned int count, unsigned int valuesPerLine, char* format) {
    for (int i = 0; i < count; i++) {
        void* p = (char*)data + (i * size);
        fprintf(dest, format, *(unsigned int*)p);
        // end of line
        if (i % valuesPerLine == (valuesPerLine - 1)) fprintf(dest, "\n");
    }
    // if last line isn't filled, add a newline
    if (count % valuesPerLine != 0) fprintf(dest, "\n");
}

static inline void htw_printVectorArray(FILE* dest, void* data, unsigned int dim, unsigned int count, unsigned int vecsPerLine, char* format) {
    for (int i = 0; i < count; i++) {
        void* p = (char*)data + (i * sizeof(float));
        fprintf(dest, format, *(unsigned int*)p);
        // end of line
        if (i % vecsPerLine == (vecsPerLine - 1)) fprintf(dest, "\n");
    }
    // if last line isn't filled, add a newline
    if (count % vecsPerLine != 0) fprintf(dest, "\n");
}

/* String and char utilities */
// Corresponds to the index of 0 in the standard character set
#define INT_CHAR_OFFSET 48
#define charToInt(c) (int)c - INT_CHAR_OFFSET
#define intToChar(i) (char)i + INT_CHAR_OFFSET

/* Math utilities */
inline int min_int(int a, int b) {
    return a < b ? a : b;
}

inline int max_int(int a, int b) {
    return a > b ? a : b;
}

int lerp_int(int a, int b, double prog);

double inverseLerp_int(int a, int b, int val);

int remap_int(int val, int oldMin, int oldMax, int newMin, int newMax);

/* Generic Lists */
typedef struct {
    int length;
    int capacity;
    int itemSize;
    void *items;
} List;

List *createList (int itemSize, int initialLength);
// Returns a new list object that refers to the same set of items as originalList. Resizing the new list... ?
List *sliceList(List *originalList, int startIndex, int sliceLength);
// Frees both the list object and the inner array
void destroyList(List *list);

// index must be less than list.length
void *getItem(List *list, int index);
// Uses memcpy to insert the value of *newItem at list[index]
void setItem(List *list, int index, void *newItem);
// Returns the index of the newly added item. Will expand the list if there is no available capacity.
int pushItem(List *list, void *newItem);
// Returns a pointer to the last item in the list, then shrinks the list. The caller is responsible for using the return value before assigning something else to the end of the list.
void *popItem(List *list);

void printList(List *list, void(*print)(void*));

// TODO
/** Generic object pools
 * Allows for frequent reuse of objects without reallocation of memory. Creating a pool allocates enough space for n objects once. The pool keeps track of which pool items are in use/not in use. Requesting a new object from the pool returns the pointer to an unused item, and marks that item as in use. Telling the pool to destroy an object marks it as unused. Destroying the pool frees all items from memory.
 */
typedef struct {
    int capacity;
    int itemSize;
    void *poolItems;
} Pool;

Pool *createPool(int itemSize, int capacity);

/**
 * @brief Free all memory used by Pool [pool]
 *
 * @param pool pool to destroy
 * @return >= 0: number of pool objects still in use. < 0: error
 */
int destroyPool(Pool *pool);
void *getNewPoolItem(Pool *pool);
int destroyPoolItem(Pool *pool, void *item);

#endif // HTW_CORE_H_INCLUDED
