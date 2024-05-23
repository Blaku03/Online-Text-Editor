#ifndef SERVER_LINKED_LIST_H
#define SERVER_LINKED_LIST_H

#include <pthread.h>
#include <stdio.h>

typedef struct Node
{
    char* content;
    char* user_name;
    int locked;
    int socket_id;
    struct Node* next;
    struct Node* previous;
} Node;

typedef struct LinkedList
{
    Node* head;
    Node* tail;
    pthread_mutex_t linked_list_mutex;
} LinkedList;

void linked_list_init(LinkedList* list);
void linked_list_destroy(LinkedList* list);
void linked_list_print(LinkedList* list);
// Create a new linked list node allocated on the heap
// with a copy of the passed content and return a pointer
// to it on success, otherwise return NULL.
Node* linked_list_create_node(const char* content);
void linked_list_destroy_node(Node* node);
// Count the number of nodes in the linked list and return it.
// Return -1 on failure.
int linked_list_get_length(LinkedList* list);
// Return 0 on success, otherwise return -1.
int linked_list_insert_before_head(LinkedList* list, Node* node);
// Return 0 on success, otherwise return -1.
int linked_list_insert_after_tail(LinkedList* list, Node* node);
// Return 0 on success, otherwise return -1.
int linked_list_insert_after_node_number(LinkedList* list, int node_number, Node* node);
// Return pointer to removed node on success, otherwise return NULL.
Node* linked_list_remove_node(LinkedList* list, int paragraph_number);
char* edit_content_of_paragraph(LinkedList* list, int paragraph_number, const char* new_content);
void refresh_file(LinkedList* list, const char* file_name);
int get_socket_id(LinkedList* list, int paragraph_number);
int is_locked(LinkedList* list, int paragraph_number);
void lock_paragraph(LinkedList* list, int paragraph_number, int socket_id, char* user_name);
void unlock_paragraph(LinkedList* list, int paragraph_number, int socket_id);
void parse_file_to_linked_list(LinkedList* list, const char* file_name);
char* get_content_of_paragraph(LinkedList* list, int paragraph_number);
// Return number of unlocked paragraph, otherwise return -1.
int unlock_paragraph_with_socket_id(LinkedList* list, int socket_id);
int get_number_of_paragraph_locked_by_given_socket(LinkedList* list, int socket_id);

#endif  // SERVER_LINKED_LIST_H
