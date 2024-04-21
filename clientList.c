#include "clientList.h"

Node* init_client_list(Client* client) {
    Node* node = malloc(sizeof(Node));
    node->client = client;
    node->next = NULL;
    return node;
}

void add_client(Node* node, Client* client) {
    Node* newNode = init_client_list(client);
    newNode->next = node->next;
    node->next = newNode;
}

Node* delete_client(Node* node, Client* client) {
    Node* previous = NULL;
    Node* result = NULL;
    do {
        if (node->client == client) {
            //First node
            if (!previous) {
                result = node->next;
                free(node);
                return result;
            }
            
            //Other nodes
            previous->next = node->next;
            free(node);
            return result;
        }
        previous = node;
    } while (node = node->next, node);
    return result;
}

bool is_last_client(Node* node) {
    return !node->next; 
}

bool in_list(Node* node, Client* client) {
    do {
        if (node->client == client) {
            return true;
        }
    } while (node = node->next, node);
    return false;
}

