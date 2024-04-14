#ifndef SERVER_LINKED_LIST_H
#define SERVER_LINKED_LIST_H

#include <pthread.h>
#include <stdio.h>

typedef struct Node {
    char* content;
    int locked;
    int socket_id;
    struct Node* next;
    struct Node* previous;
} Node;

typedef struct LinkedList {
    Node* head;
    Node* tail;
    pthread_mutex_t linked_list_mutex;
} LinkedList;

void init_linked_list(LinkedList* list);
Node* create_new_node(const char* content);
int insert_after_tail(LinkedList* list, Node* node);
int insert_before_head(LinkedList* list, Node* node);
int insert_after_node_number(LinkedList* list, int node_number, Node* node);
void delete(LinkedList* list, char* content);
void print_list(LinkedList* list);
void edit_content_of_paragraph(LinkedList* list, int paragraph_number, char* new_content);
void refresh_file(LinkedList* list, const char* file_name);
int get_socket_id(LinkedList* list, int paragraph_number);
int is_locked(LinkedList* list, int paragraph_number);
void lock_paragraph(LinkedList* list, int paragraph_number, int socket_id);
void unlock_paragraph(LinkedList* list, int paragraph_number, int socket_id);
void parse_file_to_linked_list(LinkedList* list, const char* file_name);
void free_linked_list(LinkedList* list);
char* get_content_of_paragraph(LinkedList* list, int paragraph_number);
void unlock_paragraph_with_socket_id(LinkedList* list, int socket_id);
int get_number_of_nodes(LinkedList* list);


#endif//SERVER_LINKED_LIST_H
