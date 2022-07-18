#ifndef HTW_CORE_H_INCLUDED
#define HTW_CORE_H_INCLUDED

#include <stdlib.h>

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


#endif // HTW_CORE_H_INCLUDED
