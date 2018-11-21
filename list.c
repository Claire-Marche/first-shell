#include <stdlib.h>
#include <stdio.h>

#include "list.h"

void list_create(struct list *l) {
    l->first = NULL;
}

void node_create(struct node *ne, int n) {
    ne->value = n;
    ne->next = NULL;
}

void list_add(struct list *l, int n) {
    struct node *ne = malloc(sizeof(struct node));
    node_create(ne, n);
    ne->next = l->first;
    l->first = ne;
}

void list_destroy(struct list *l) {    
    struct node *curr = l->first;
    struct node *prev = curr;
    
    while (curr != NULL) {
        free(prev);
        prev = curr;
        curr = curr->next;
    }
}
