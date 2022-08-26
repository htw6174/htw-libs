#include <stdio.h>
#include <string.h>
#include "htw_core.h"

#define GET_ITEM_POINTER(list, index) (list->items + (index * list->itemSize))

List *createList (int itemSize, int initialLength) {
    void *items = malloc(itemSize * initialLength);
    // malloc l instead of using struct assignment to avoid returning the address of a local variable
    List *l = (List*)malloc(sizeof(List));
    l->items = items;
    l->length = initialLength;
    l->capacity = initialLength;
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

// Returns new capacity of the list
static int addCapacity(List *list) {
    int newCapacity = list->capacity * 2;
    if (newCapacity == 0)
        newCapacity = 1;
    list->items = realloc(list->items, newCapacity * list->itemSize);
    list->capacity = newCapacity;
    return newCapacity;
}

void destroyList(List *list) {
    free(list->items);
    free(list);
}

// TODO: add out of range check
// return a pointer to the item at the given index
void *getItem(List *list, int index) {
    return GET_ITEM_POINTER(list, index);
}

// Uses memcpy to add the value of *newItem to list[index]
void setItem(List *list, int index, void *newItem) {
    memcpy(GET_ITEM_POINTER(list, index), newItem, list->itemSize);
}

// Returns the index of the newly added item
int pushItem(List *list, void *newItem) {
    if (list->length == list->capacity)
        addCapacity(list);
    memcpy(GET_ITEM_POINTER(list, list->length), newItem, list->itemSize);
    list->length++;
    return list->length - 1;
}

// Returns a pointer to the last item in the list, then shrinks the list. The caller is responsible for using the return value before assigning something else to the end of the list.
void *popItem(List *list) {
    void *popped = getItem(list, list->length - 1);
    list->length--;
    return popped;
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
