#ifndef HTW_CORE_H_INCLUDED
#define HTW_CORE_H_INCLUDED

#include <stdlib.h>

typedef struct {
    int length;
    int itemSize;
    void *items;
} List;

List *createList (int itemSize, int initialLength);
List *sliceList(List *originalList, int startIndex, int sliceLength);
void destroyList(List *list);

void *getItem(List *list, int index);
void setItem(List *list, int index, void *newItem);

void printList(List *list, void(*print)(void*));


#endif // HTW_CORE_H_INCLUDED
