#include "linked_list.h"

#include <stdlib.h>
#include <string.h>

void init_linked_list(LinkedList* list) {
    list->head = NULL;
    list->tail = NULL;
    pthread_mutex_init(&list->linked_list_mutex, NULL);
}

// Create a new linked list node allocated on the heap
// with a copy of the passed content and return a pointer
// to it on success, otherwise return NULL.
Node* create_new_node(const char* content) {
    if (content == NULL) {
        return NULL;
    }
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        return NULL;
    }
    char* content_copy = strdup(content);
    if (content_copy == NULL) {
        free(new_node);
        return NULL;
    }
    new_node->content = content_copy;
    new_node->locked = 0;
    new_node->socket_id = -1;
    new_node->next = NULL;
    new_node->previous = NULL;
    return new_node;
}

int insert_after_tail(LinkedList* list, Node* node) {
    if (node == NULL) {
        return -1;
    }
    pthread_mutex_lock(&list->linked_list_mutex);
    node->previous = list->tail;

    if (list->tail != NULL) {
        list->tail->next = node;
    }
    list->tail = node;
    if (list->head == NULL) {
        list->head = node;
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
    return 0;
}

int insert_after_node_number(LinkedList* list, int node_number, Node* node) {
    if (node == NULL) {
        return -1;
    }
    pthread_mutex_lock(&list->linked_list_mutex);
    node->next = NULL;
    node->previous = NULL;

    Node* current_node = list->head;
    int current_number = 1;
    while (current_node != NULL && current_number < node_number) {
        current_node = current_node->next;
        current_number++;
    }

    if (current_node != NULL) {
        node->next = current_node->next;
        node->previous = current_node;
        if (current_node->next != NULL) {
            current_node->next->previous = node;
        }
        current_node->next = node;
        if (list->tail == current_node) {
            list->tail = node;
        }
    } else {
        list->head = node;
        list->tail = node;
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
    return 0;
}

int insert_before_head(LinkedList* list, Node* node) {
    if (node == NULL) {
        return -1;
    }
    pthread_mutex_lock(&list->linked_list_mutex);
    node->previous = NULL;
    if (list->head != NULL) {
        list->head->previous = node;
    }
    list->head = node;
    if (list->tail == NULL) {
        list->tail = node;
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
    return 0;
}


void delete_node(LinkedList* list, int paragraph_number){
    pthread_mutex_lock(&list->linked_list_mutex);
    Node* temp = list->head;
    int current_number = 1;
    while (temp != NULL && current_number < paragraph_number) {
        temp = temp->next;
        current_number++;
    }
    if (temp != NULL) {
        if (temp->previous != NULL) {
            temp->previous->next = temp->next;
        } else {
            list->head = temp->next;
        }
        if (temp->next != NULL) {
            temp->next->previous = temp->previous;
        } else {
            list->tail = temp->previous;
        }
        free(temp);
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
}

void print_list(LinkedList* list) {
    Node* temp = list->head;
    while (temp != NULL) {
        printf("Content: %s, Locked: %d, SocketID: %d\n", temp->content, temp->locked, temp->socket_id);
        temp = temp->next;
    }
    fflush(stdout); //added for debugging because when debugging with GDB printf is buffered
}

void lock_paragraph(LinkedList* list, int paragraph_number, int socket_id) {
    pthread_mutex_lock(&list->linked_list_mutex);
    Node* temp = list->head;
    int current_number = 1;
    while (temp != NULL && current_number < paragraph_number) {
        temp = temp->next;
        current_number++;
    }
    if (temp != NULL) {
        temp->locked = 1;
        temp->socket_id = socket_id;
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
}

int is_locked(LinkedList* list, int paragraph_number) {
    pthread_mutex_lock(&list->linked_list_mutex);
    Node* temp = list->head;
    int current_number = 1;
    while (temp != NULL && current_number < paragraph_number) {
        temp = temp->next;
        current_number++;
    }
    if (temp != NULL) {
        pthread_mutex_unlock(&list->linked_list_mutex);
        return temp->locked;
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
    return -1; // Return -1 if the paragraph is not found
}

int get_socket_id(LinkedList* list, int paragraph_number) {
    pthread_mutex_lock(&list->linked_list_mutex);
    Node* temp = list->head;
    int current_number = 1;
    while (temp != NULL && current_number < paragraph_number) {
        temp = temp->next;
        current_number++;
    }
    if (temp != NULL) {
        pthread_mutex_unlock(&list->linked_list_mutex);
        return temp->socket_id;
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
    return -1; // Return -1 if the paragraph is not found
}

void parse_file_to_linked_list(LinkedList* list, const char* file_name) {
    FILE* file = fopen(file_name, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: file not found\n");
        return;
    }

    char line[1024]; //TODO: is it enough for paragraph ?? to consider
    char last_char;
    while (fgets(line, sizeof(line), file)) {
        insert_after_tail(list, create_new_node(line));
        last_char = line[strlen(line) - 1];
    }

    // Check if the last line ended with a newline character
    // if yes it means that last line is empty, so insert it with empty content
    if (last_char == '\n') {
        insert_after_tail(list, create_new_node(""));
    }

    fclose(file);
}

void free_linked_list(LinkedList* list) {
    Node* temp = list->head;
    while (temp != NULL) {
        Node* next = temp->next;
        free(temp->content);
        temp->content = NULL;
        free(temp);
        temp = next;
    }
    list->head = NULL;
    list->tail = NULL;
    pthread_mutex_destroy(&list->linked_list_mutex);
}

void unlock_paragraph(LinkedList* list, int paragraph_number, int socket_id) {
    pthread_mutex_lock(&list->linked_list_mutex);
    Node* temp = list->head;
    int current_number = 1;
    while (temp != NULL && current_number < paragraph_number) {
        temp = temp->next;
        current_number++;
    }
    if (temp != NULL && temp->socket_id == socket_id) {
        temp->locked = 0;
        temp->socket_id = -1;
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
}

void edit_content_of_paragraph(LinkedList* list, int paragraph_number, char* new_content) {
    pthread_mutex_lock(&list->linked_list_mutex);
    Node* temp = list->head;
    int current_number = 1;
    while (temp != NULL && current_number < paragraph_number) {
        temp = temp->next;
        current_number++;
    }
    if (temp != NULL) {
        temp->content = strdup(new_content);
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
}

void refresh_file(LinkedList* list, const char* file_name) {
    pthread_mutex_lock(&list->linked_list_mutex);
    FILE* file = fopen(file_name, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: file not found\n");
        return;
    }

    int number_of_nodes = get_number_of_nodes(list);
    int printed_nodes = 0;

    Node* temp = list->head;
    while (temp != NULL) {
        fprintf(file, "%s", temp->content);
        printed_nodes++;
        temp = temp->next;
        if(printed_nodes == number_of_nodes-1) break;
    }

    // deleting '\n' from last node in case of having empty last line in initial file
    if(temp != NULL){
        char* content_of_last_node = temp->content;
        content_of_last_node[strcspn(content_of_last_node, "\n")] = '\0';
        fprintf(file, "%s", temp->content);
    }

    fclose(file);
    pthread_mutex_unlock(&list->linked_list_mutex);
}

char* get_content_of_paragraph(LinkedList* list, int paragraph_number) {
    pthread_mutex_lock(&list->linked_list_mutex);
    Node* temp = list->head;
    int current_number = 1;
    while (temp != NULL && current_number < paragraph_number) {
        temp = temp->next;
        current_number++;
    }
    if (temp != NULL) {
        pthread_mutex_unlock(&list->linked_list_mutex);
        return temp->content;
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
    return NULL;
}

void unlock_paragraph_with_socket_id(LinkedList* list, int socket_id) {
    pthread_mutex_lock(&list->linked_list_mutex);
    Node* temp = list->head;
    while (temp != NULL) {
        if (temp->socket_id == socket_id) {
            temp->locked = 0;
            temp->socket_id = -1;
        }
        temp = temp->next;
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
}
int get_number_of_nodes(LinkedList* list) {
    Node* temp = list->head;
    int count = 0;
    while (temp != NULL) {
        count++;
        temp = temp->next;
    }
    return count;
}
