#include "htw_core.h"

List *createList (int itemSize, int initialLength) {
    void *items = malloc(itemSize * initialLength);
    // malloc l instead of using struct assignment to avoid returning the address of a local variable
    List *l = (List*)malloc(sizeof(List));
    l->items = items;
    l->length = initialLength;
    l->itemSize = itemSize;
    return l;
}

// TODO: do something to indicate an error when out of range
List *sliceList(List *originalList, int startIndex, int sliceLength) {
    if (startIndex >= originalList->length || startIndex + sliceLength > originalList->length) {
        return NULL;
    }
    List *l = (List*)malloc(sizeof(List));
    void *newStart = getItem(originalList, startIndex);
    l->items = newStart;
    l->length = sliceLength;
    l->itemSize = originalList->itemSize;
    return l;
}

void destroyList(List *list) {
    free(list);
}

// TODO: add out of range check
// return a pointer to the item at the given index
void *getItem(List *list, int index) {
    return list->items + (index * list->itemSize);
}

// any way to make this work if item type is unknown?
void setItem(List *list, int index, void *newItem) {
    int *item = (int*)getItem(list, index); // = (int*)newItem;
    *item = *(int*)newItem;
}

void printList(List *list, void(*print)(void*)) {
    printf("[");
    print(getItem(list, 0));
    for(int i = 1; i < list->length; i++) {
        printf(", ");
        print(getItem(list, i));
    }
    printf("]\n");
}
