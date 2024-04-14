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
    free(locked_paragraphs_message);
    locked_paragraphs_message = NULL;

    recv(sock, client_message, sizeof(client_message), 0);
    if (strcmp(client_message, "OK") != 0) {
        fprintf(stderr, "Error: client response not OK\n");
    }

    //    add_socket(sock);             //adding socket to connected_sockets array
    modify_socket_array(-1, sock);

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

        //deleting protocol_id from client message
        memmove(client_message, client_message + 2, strlen(client_message) - 1);


        switch (protocol_id) {
            case SYNC_PROTOCOL_ID:
                update_paragraph_protocol(sock, paragraphs, client_message, SYNC_PROTOCOL_ID);
                break;
            case ASYNC_PROTOCOL_ADD_PARAGRAPH_ID:
                async_protocol_new_paragraph(sock, paragraphs, client_message, 1);
                break;
            case ASYNC_PROTOCOL_DELETE_PARAGRAPH_ID:
                async_protocol_delete_paragraph(sock, paragraphs, 0);
                break;
            case UNLOCK_PARAGRAPH_PROTOCOL_ID:
                update_paragraph_protocol(sock, paragraphs, client_message, UNLOCK_PARAGRAPH_PROTOCOL_ID);
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
    //    remove_socket(sock);// remove socket from connected_sockets array
    modify_socket_array(sock, -1);
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
    // getting paragraph number
    int paragraph_number;
    if (sscanf(client_message, "%d", &paragraph_number) != 1) {
        fprintf(stderr, "Error: could not parse paragraph number from message\n");
        return;
    }
    memmove(client_message, client_message + 2, strlen(client_message) - 1);
    char *paragraph_content = NULL;
    switch (protocol_id) {
        case SYNC_PROTOCOL_ID:
            lock_paragraph(paragraphs, paragraph_number, sock);
            paragraph_content = edit_content_of_paragraph(paragraphs, paragraph_number, client_message);
            break;
        case UNLOCK_PARAGRAPH_PROTOCOL_ID:
            unlock_paragraph(paragraphs, paragraph_number, sock);
            paragraph_content = get_content_of_paragraph(paragraphs, paragraph_number);
            break;
    }

    // remove \n from paragraph content

    // Create the message
    char message[KILOBYTE];
    char dbg_message[KILOBYTE];
    snprintf(message, sizeof(message), "%d,%d,%s", protocol_id, paragraph_number, paragraph_content);
    snprintf(dbg_message, sizeof(message), "%s,%d,%s", get_protocol_name(protocol_id), paragraph_number, paragraph_content);
    fprintf(stderr, "SocketID: %d, ClientMessage: %s \n", sock, dbg_message);
    fflush(stdout);
    refresh_file(paragraphs, FILE_NAME);
    broadcast(message, sock);
}

void async_protocol_new_paragraph(int sock, LinkedList *paragraphs, char *client_message, int insert_after) {
    // TODO: extract paragraph number from client message
    //    insert_after_node_number(paragraphs, add_after, client_message);
    //    refresh_file(paragraphs, FILE_NAME);
    //    broadcast(client_message, sock);
}

void async_protocol_delete_paragraph(int sock, LinkedList *paragraphs, int paragraph_number) {
    //TODO
}

void modify_socket_array(int search_for, int replace_with) {
    pthread_mutex_lock(&connected_sockets_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (connected_sockets[i] == search_for) {
            connected_sockets[i] = replace_with;
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
    int current_number_itr = 1;
    int locked_paragraphs_count = 0;

    // count the number of locked paragraphs
    while (temp != NULL) {
        if (temp->locked == 1) {
            locked_paragraphs_count++;
        }
        temp = temp->next;
    }
    temp = paragraphs->head;

    // Allocate memory for the message
    // Each number will be at most 10 characters long, plus one for the comma
    char *message = malloc(locked_paragraphs_count * 11 + 2);// +2 for the "0" and the null terminator
    if (message == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return NULL;
    }

    message[0] = '\0';
    while (temp != NULL) {
        if (temp->locked == 1) {
            char number[11];// Buffer to hold the string representation of the number
            sprintf(number, "%d,", current_number_itr);
            strcat(message, number);
        }
        temp = temp->next;
        current_number_itr++;
    }

    strcat(message, "0");// Add a zero to indicate the end of the message
    return message;
}

const char *get_protocol_name(int protocol_id) {
    switch (protocol_id) {
        case SYNC_PROTOCOL_ID:
            return "SYNC PROTOCOL";
        case ASYNC_PROTOCOL_ADD_PARAGRAPH_ID:
            return "ADD PARAGRAPH";
        case ASYNC_PROTOCOL_DELETE_PARAGRAPH_ID:
            return "DELETE PARAGRAPH";
        case UNLOCK_PARAGRAPH_PROTOCOL_ID:
            return "UNLOCK PARAGRAPH";
        default:
            return "UNKNOWN PROTOCOL";
    }
}