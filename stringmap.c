#include "stringmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct Node {
    StringMapItem* itemMap;
    struct Node* next;
};

typedef struct Node Node;

struct StringMap {
    Node* firstNode;
};

StringMap* stringmap_init() {
    StringMap* map = malloc(sizeof(StringMap));
    map->firstNode = NULL;
    return map;
}

void stringmap_free(StringMap* sm) {
    if (!sm) {
        return;
    }
    
    Node* node = sm->firstNode;
    while (node) {
        Node* next = node->next;
        free(node->itemMap->key);
        free(node->itemMap);
        free(node);
        node = next;
    } 
    free(sm);
}

void* stringmap_search(StringMap* sm, char* key) {
    if (!sm || !key) {
        return NULL;
    }

    Node* node = sm->firstNode;
    while (node) {
        Node* next = node->next;
        if (!strcmp(key, node->itemMap->key)) {
            return node->itemMap->item;
        }
        node = next;
    } 
    return NULL;
}

int stringmap_add(StringMap* sm, char* key, void* item) {
    if (!sm || !key || !item) {
        return 0;
    }

    //Ensure key is not already in the stringmap
    if (stringmap_search(sm, key)) {
        return 0;
    }
    //Setup new node with its item map
    Node* newNode = malloc(sizeof(Node));
    StringMapItem* newItemMap = malloc(sizeof(StringMapItem));
    newItemMap->key = strdup(key);
    newItemMap->item = item;
    newNode->itemMap = newItemMap;
    
    //Add new node to string map
    if (!sm->firstNode) {
        //Map is empty
        sm->firstNode = newNode;

    } else {
        //Map has node
        Node* next = sm->firstNode->next;
        sm->firstNode->next = newNode;
        newNode->next = next;
    }
    return 1;
}

int stringmap_remove(StringMap* sm, char* key) {
    if (!sm || !key) {
        return 0;
    }

    Node* node = sm->firstNode;
    Node* previous = NULL;
    while (node) {
        Node* next = node->next;
        if (!strcmp(key, node->itemMap->key)) {
            //Fix connections
            if (!previous) {
                //If first element removed, set first node
                sm->firstNode = node->next;
            } else {
                previous->next = node->next;
            }

            //Free
            free(node->itemMap->key);
            free(node->itemMap);
            free(node);
            return 1;
        }
        previous = node;
        node = next;
    }
    return 0;
}

StringMapItem* stringmap_iterate(StringMap* sm, StringMapItem* prev) {
    if (!sm || !sm->firstNode) {
        return NULL;
    }
    
    if (!prev) {
        return sm->firstNode->itemMap;
    }
    
    Node* node = sm->firstNode;
    while (node) {
        Node* next = node->next;
        if (!strcmp(prev->key, node->itemMap->key) && next) {
            return next->itemMap;
        }   
        node = next;
    }
    return NULL;
}
