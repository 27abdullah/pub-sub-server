#ifndef CLIENTLIST_H
#define CLIENTLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

//Struct that stores the data necessary to represent a client
typedef struct {
    char* name;
    bool hasName;
    FILE* in;
    FILE* out;
} Client;

//A node in the linked list that can hold all clients
struct Node {
    Client* client;
    struct Node* next;
};

typedef struct Node Node;

/* init_client_list()
 * ------------------
 * Initialises the client linked list
 *
 * client: the first client in the list
 *
 * Returns: the first node in the linked list
 */
Node* init_client_list(Client* client);

/* add_client()
 * ------------
 * Adds a client to the linked list of clients
 *
 * node: the first node in the linked list
 *
 * client: the client to be added
 */
void add_client(Node* node, Client* client);

/* delete_client()
 * ---------------
 * Deletes a client from the linked list with same memory address as the
 * provided client
 *
 * node: the first node in the client linked list
 *
 * client: the client to be deleted
 *
 * Returns: NULL if the client is not deleted or the client is successfully
 * deleted, and returns a pointer to the new first Node if the first client 
 * was deleted.
 */
Node* delete_client(Node* node, Client* client);

/* is_last_client()
 * ----------------
 * Determines if there are more clients in the linked list
 *
 * node: a pointer to the first element in the linked list
 * 
 * Returns: true if the client is the last client and false otherwise
 */
bool is_last_client(Node* node);

/* in_list()
 * ---------
 * Determines if the provided client is in the linked list
 *
 * node: a pointer to the first node in the linked list
 *
 * client: the client being checked
 *
 * Returns: true if the client is in the list and false if it is not.
 */
bool in_list(Node* node, Client* client);
#endif

