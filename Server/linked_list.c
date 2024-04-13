#include "linked_list.h"
#include <stdlib.h>
#include <string.h>

pthread_mutex_t linked_list_mutex;

void initLinkedList(LinkedList *list) {
    list->head = NULL;
    list->tail = NULL;
    pthread_mutex_init(&list->linked_list_mutex, NULL);
}

void insert_after_tail(LinkedList *list, char *content) {
    pthread_mutex_lock(&list->linked_list_mutex);
    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->content = strdup(content);
    new_node->locked = 0;
    new_node->socket_id = -1;
    new_node->next = NULL;
    new_node->previous = list->tail;

    if (list->tail != NULL) {
        list->tail->next = new_node;
    }
    list->tail = new_node;
    if (list->head == NULL) {
        list->head = new_node;
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
}

void insert_after_node_number(LinkedList *list, int node_number, char *content) {
    pthread_mutex_lock(&list->linked_list_mutex);
    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->content = strdup(content);
    new_node->locked = 0;
    new_node->socket_id = -1;
    new_node->next = NULL;
    new_node->previous = NULL;

    Node *current_node = list->head;
    int current_number = 1;
    while (current_node != NULL && current_number < node_number) {
        current_node = current_node->next;
        current_number++;
    }

    if (current_node != NULL) {
        new_node->next = current_node->next;
        new_node->previous = current_node;
        if (current_node->next != NULL) {
            current_node->next->previous = new_node;
        }
        current_node->next = new_node;
        if (list->tail == current_node) {
            list->tail = new_node;
        }
    } else {
        list->head = new_node;
        list->tail = new_node;
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
}

void insert_before_head(LinkedList *list, char *content) {
    pthread_mutex_lock(&list->linked_list_mutex);
    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->content = strdup(content);
    new_node->locked = 0;
    new_node->socket_id = -1;
    new_node->next = list->head;
    new_node->previous = NULL;
    if (list->head != NULL) {
        list->head->previous = new_node;
    }
    list->head = new_node;
    if (list->tail == NULL) {
        list->tail = new_node;
    }
    pthread_mutex_unlock(&list->linked_list_mutex);
}


void delete(LinkedList* list, char *content) {
    pthread_mutex_lock(&linked_list_mutex);
    Node* temp = list->head;
    Node* prev = NULL;

    while (temp != NULL && strcmp(temp->content, content) != 0) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) {
        pthread_mutex_unlock(&linked_list_mutex);
        return;
    }

    if (prev != NULL) prev->next = temp->next;
    else list->head = temp->next;

    if (temp->next != NULL) temp->next->previous = prev;
    else list->tail = prev;

    free(temp->content);
    free(temp);
    pthread_mutex_unlock(&linked_list_mutex);
}

void print_list(LinkedList* list) {
    Node* temp = list->head;
    while (temp != NULL) {
        printf("Content: %s, Locked: %d, SocketID: %d\n", temp->content, temp->locked, temp->socket_id);
        temp = temp->next;
    }
    fflush(stdout);//added for debugging because when debugging with GDB printf is buffered
}

void lock_paragraph(LinkedList* list, int paragraph_number, int socket_id) {
    pthread_mutex_lock(&linked_list_mutex);
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
    pthread_mutex_unlock(&linked_list_mutex);
}

int is_locked(LinkedList* list, int paragraph_number) {
    pthread_mutex_lock(&linked_list_mutex);
    Node* temp = list->head;
    int current_number = 1;
    while (temp != NULL && current_number < paragraph_number) {
        temp = temp->next;
        current_number++;
    }
    if (temp != NULL) {
        pthread_mutex_unlock(&linked_list_mutex);
        return temp->locked;
    }
    pthread_mutex_unlock(&linked_list_mutex);
    return -1; // Return -1 if the paragraph is not found
}

int get_socket_id(LinkedList* list, int paragraph_number) {
    pthread_mutex_lock(&linked_list_mutex);
    Node* temp = list->head;
    int current_number = 1;
    while (temp != NULL && current_number < paragraph_number) {
        temp = temp->next;
        current_number++;
    }
    if (temp != NULL) {
        pthread_mutex_unlock(&linked_list_mutex);
        return temp->socket_id;
    }
    pthread_mutex_unlock(&linked_list_mutex);
    return -1; // Return -1 if the paragraph is not found
}

void parse_file_to_linked_list(LinkedList* list, const char *file_name) {
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: file not found\n");
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        insert_after_tail(list, line);
    }

    fclose(file);
}

void free_linked_list(LinkedList* list) {
    Node* temp = list->head;
    while (temp != NULL) {
        Node* next = temp->next;
        free(temp->content);
        free(temp);
        temp = next;
    }
    list->head = NULL;
    list->tail = NULL;
}

void unlock_paragraph(LinkedList *list, int paragraph_number, int socket_id) {
    pthread_mutex_lock(&linked_list_mutex);
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
    pthread_mutex_unlock(&linked_list_mutex);
}

void edit_content_of_paragraph(LinkedList *list, int paragraph_number, char *new_content) {
    pthread_mutex_lock(&linked_list_mutex);
    Node* temp = list->head;
    int current_number = 1;
    while (temp != NULL && current_number < paragraph_number) {
        temp = temp->next;
        current_number++;
    }
    if (temp != NULL) {
        free(temp->content);
        temp->content = strdup(new_content);
    }
    pthread_mutex_unlock(&linked_list_mutex);

}

void refresh_file(LinkedList *list, const char *file_name) {
    pthread_mutex_lock(&linked_list_mutex);
    FILE *file = fopen(file_name, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: file not found\n");
        return;
    }

    Node* temp = list->head;
    while (temp != NULL) {
        fprintf(file, "%s", temp->content);
        temp = temp->next;
    }

    fclose(file);
    pthread_mutex_unlock(&linked_list_mutex);
}

char *get_content_of_paragraph(LinkedList *list, int paragraph_number) {
    pthread_mutex_lock(&linked_list_mutex);
    Node* temp = list->head;
    int current_number = 1;
    while (temp != NULL && current_number < paragraph_number) {
        temp = temp->next;
        current_number++;
    }
    if (temp != NULL) {
        pthread_mutex_unlock(&linked_list_mutex);
        return temp->content;
    }
    pthread_mutex_unlock(&linked_list_mutex);
    return NULL;
}

void unlock_paragraph_with_socket_id(LinkedList *list, int socket_id) {
    pthread_mutex_lock(&linked_list_mutex);
    Node* temp = list->head;
    while (temp != NULL) {
        if (temp->socket_id == socket_id) {
            temp->locked = 0;
            temp->socket_id = -1;
        }
        temp = temp->next;
    }
    pthread_mutex_unlock(&linked_list_mutex);

}
