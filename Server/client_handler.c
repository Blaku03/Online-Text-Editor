#include "client_handler.h"

int connected_sockets[MAX_CLIENTS];
pthread_mutex_t connected_sockets_mutex;

void *connection_handler(void *args) {
    //casting args
    connection_handler_args *actual_args = (connection_handler_args *) args;
    int sock = actual_args->socket_desc;
    const char *file_name = actual_args->file_name;
    LinkedList *paragraphs = actual_args->paragraphs;

    char client_message[CHUNK_SIZE];

    if (send_file_to_client(sock, file_name) < 0) {
        fprintf(stderr, "Error: sending file failed\n");
    }

    // sending information which paragraphs are locked when client connects
    char *locked_paragraphs_message = create_message_with_lock_status(paragraphs);
    send(sock, locked_paragraphs_message, strlen(locked_paragraphs_message), 0);
    recv(sock, client_message, sizeof(client_message), 0);
    if (strcmp(client_message, "OK") != 0) {
        fprintf(stderr, "Error: client response not OK\n");
    }

    add_socket(sock);//adding socket to connected_sockets array

    for (;;) {
        unsigned int read_size = recv(sock, client_message, CHUNK_SIZE, 0);
        client_message[read_size] = '\0';

        //when client disconnects
        if (read_size == 0) {
            break;
        }

        //reading protocol id
        int protocol_id = -1;
        if (sscanf(client_message, "%d", &protocol_id) != 1) {
            fprintf(stderr, "Error: could not parse ID from message\n");
            continue;
        }

        delete_id_from_client_message(client_message, protocol_id);

        switch (protocol_id) {
            case SYNC_PROTOCOL_ID:
                update_paragraph_protocol(sock, paragraphs, client_message, SYNC_PROTOCOL_ID);
                break;
            case ASYNC_PROTOCOL_ADD_PARAGRAPH_ID:
                update_paragraph_protocol(sock, paragraphs, client_message, ASYNC_PROTOCOL_ADD_PARAGRAPH_ID);
                break;
            case ASYNC_PROTOCOL_DELETE_PARAGRAPH_ID:
                async_protocol_delete_paragraph(sock, paragraphs, client_message);
                break;
            case UNLOCK_PARAGRAPH_PROTOCOL_ID:
                update_paragraph_protocol(sock, paragraphs, client_message, UNLOCK_PARAGRAPH_PROTOCOL_ID);
                break;
            case CHANGE_LINE_VIA_MOUSE_PROTOCOL_ID:
                unlock_paragraph_after_mouse_press(sock, paragraphs, client_message);
                break;
        }

        /* Clear the message buffer */
        memset(client_message, 0, CHUNK_SIZE);
    }

    unlock_paragraph_with_socket_id(paragraphs, sock);
    // TODO: create protocol to unlock paragraph with socket_id when disconnecting
    fprintf(stderr, "Client disconnected\n");

    // Free the socket pointer
    free(args);
    remove_socket(sock);// remove socket from connected_sockets array
    close(sock);
    pthread_exit(NULL);
}


int send_file_to_client(int sock, const char *file_name) {
    // Open the file
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: file not found\n");
        return -1;
    }

    // Send metadata to the client
    char metadata[KILOBYTE];
    snprintf(metadata, sizeof(metadata), "%d,%d", get_file_size(file), CHUNK_SIZE);
    send(sock, metadata, sizeof(metadata), 0);
    // printf("Metadata: %s\n", metadata);
    // free(file_name_wo_extension);

    // Waiting for confirmation from client
    char client_response[KILOBYTE];
    recv(sock, client_response, sizeof(client_response), 0);

    if (strcmp(client_response, "OK") != 0) {
        fprintf(stderr, "Error: client response not OK\n");
        fclose(file);
        return -1;
    }

    char data_buffer[CHUNK_SIZE];
    unsigned long bytes_read;
    // Fread: read data from file then assign to data_buffer
    // return value: number of elements read
    // if return value is 0, it means the end of file
    while ((bytes_read = fread(data_buffer, 1, sizeof(data_buffer), file)) > 0) {
        if (send(sock, data_buffer, bytes_read, 0) < 0) {
            fprintf(stderr, "Error: send failed\n");
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

int get_file_size(FILE *file) {
    fseek(file, 0, SEEK_END);// Move the file pointer to the end of the file
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);// Move the file pointer to the beginning of the file
    return size;
}

void update_paragraph_protocol(int sock, LinkedList *paragraphs, char *client_message, int protocol_id) {
    int paragraph_number = extract_paragraph_number(client_message);

    switch (protocol_id) {
        case SYNC_PROTOCOL_ID:
            lock_paragraph(paragraphs, paragraph_number, sock);
            break;
        case UNLOCK_PARAGRAPH_PROTOCOL_ID:
            unlock_paragraph(paragraphs, paragraph_number, sock);
            break;
        case ASYNC_PROTOCOL_ADD_PARAGRAPH_ID:
            unlock_paragraph(paragraphs, paragraph_number, sock);
            insert_after_node_number(paragraphs, paragraph_number, create_new_node("\n"));
            lock_paragraph(paragraphs, paragraph_number + 1, sock);
            break;
    }

    edit_content_of_paragraph(paragraphs, paragraph_number, client_message);
    char *paragraph_content = get_content_of_paragraph(paragraphs, paragraph_number);

    // Create the message
    char message[KILOBYTE];
    snprintf(message, sizeof(message), "%d,%d,%s", protocol_id, paragraph_number, paragraph_content);
    fprintf(stderr, "SocketID: %d, ClientMessage: %s \n", sock, message);
    fflush(stdout);
    refresh_file(paragraphs, FILE_NAME);
    broadcast(message, sock);
}

int extract_paragraph_number(char* client_message){
    int paragraph_number;
    if (sscanf(client_message, "%d", &paragraph_number) != 1) {
        fprintf(stderr, "Error: could not parse paragraph number from message\n");
    }

    // deleting paragraph number based on its length
    char paragraph_number_as_string[20];
    sprintf(paragraph_number_as_string, "%d", paragraph_number);
    unsigned int digits_of_paragraph_number = strlen(paragraph_number_as_string);
    memmove(client_message, client_message + digits_of_paragraph_number + 1, strlen(client_message) - digits_of_paragraph_number);

    return paragraph_number;
}

void delete_id_from_client_message(char* client_message, int id){
    // deleting id based on its length
    char id_as_string[20];
    sprintf(id_as_string, "%d", id);
    unsigned int digits_of_paragraph_number = strlen(id_as_string);
    memmove(client_message, client_message + digits_of_paragraph_number + 1, strlen(client_message) - digits_of_paragraph_number);
}

void async_protocol_delete_paragraph(int sock, LinkedList *paragraphs, char* client_message){
    int paragraph_number = extract_paragraph_number(client_message);
    char* content_of_paragraph_to_delete = get_content_of_paragraph(paragraphs, paragraph_number);
    char* content_of_paragraph_above = get_content_of_paragraph(paragraphs, paragraph_number - 1);
    content_of_paragraph_above[strcspn(content_of_paragraph_above, "\n")] = '\0'; //trim '\n'

    size_t new_size = strlen(content_of_paragraph_above) + strlen(content_of_paragraph_to_delete) + 1;
    char *reallocated_content = realloc(content_of_paragraph_above, new_size);
    if (reallocated_content == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return;
    }
    content_of_paragraph_above = reallocated_content;
    strcat(content_of_paragraph_above, content_of_paragraph_to_delete);

    edit_content_of_paragraph(paragraphs, paragraph_number - 1, content_of_paragraph_above);
    delete_node(paragraphs, paragraph_number);

    // Create the message
    char message[KILOBYTE];
    snprintf(message, sizeof(message), "%d,%d", ASYNC_PROTOCOL_DELETE_PARAGRAPH_ID, paragraph_number);
    fprintf(stderr, "SocketID: %d, ClientMessage: %s \n", sock, message);
    fflush(stdout);
    refresh_file(paragraphs, FILE_NAME);
    broadcast(message, sock);
}

void add_socket(int socket) {
    pthread_mutex_lock(&connected_sockets_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (connected_sockets[i] == -1) {
            connected_sockets[i] = socket;
            break;
        }
    }
    pthread_mutex_unlock(&connected_sockets_mutex);
}

void remove_socket(int socket) {
    pthread_mutex_lock(&connected_sockets_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (connected_sockets[i] == socket) {
            connected_sockets[i] = -1;
            break;
        }
    }
    pthread_mutex_unlock(&connected_sockets_mutex);
}

void broadcast(char *message, int sender) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (connected_sockets[i] != -1 && connected_sockets[i] != sender) {
            send(connected_sockets[i], message, strlen(message), 0);
        }
    }
}

char *create_message_with_lock_status(LinkedList *paragraphs) {
    Node *temp = paragraphs->head;
    int current_number = 1;
    char *message = malloc(1);
    message[0] = '\0';

    while (temp != NULL) {
        if (temp->locked == 1) {
            char number[10];
            sprintf(number, "%d,", current_number);
            char *reallocated_message = realloc(message, strlen(message) + strlen(number) + 1);
            if (reallocated_message == NULL) {
                fprintf(stderr, "Error: memory allocation failed\n");
                free(message);
                return NULL;
            }
            message = reallocated_message;
            strcat(message, number);
        }
        temp = temp->next;
        current_number++;
    }

    // if it's first client, all paragraphs will be unlocked so return 0
    if (strcmp(message, "") == 0) {
        strcat(message, "0");
    } else {
        // adding '0' to indicate end of message if some paragraphs are locked
        char *reallocated_message = realloc(message, strlen(message) + 2);
        if (reallocated_message == NULL) {
            fprintf(stderr, "Error: memory allocation failed\n");
            free(message);
            return NULL;
        }
        message = reallocated_message;
        strcat(message, "0");
    }
    message[strlen(message)] = '\0';
    return message;
}

void unlock_paragraph_after_mouse_press(int sock, LinkedList *paragraphs, char *client_message) {
    int paragraph_number = extract_paragraph_number(client_message);
    char message[KILOBYTE];
    snprintf(message, sizeof(message), "%d,%d", CHANGE_LINE_VIA_MOUSE_PROTOCOL_ID, paragraph_number);
    unlock_paragraph(paragraphs, paragraph_number, sock);
    broadcast(message, sock);
}
