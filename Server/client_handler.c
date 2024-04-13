#include "client_handler.h"

int connected_sockets[MAX_CLIENTS];
pthread_mutex_t connected_sockets_mutex;

void *connection_handler(void* args)
{
    //casting args
    connection_handler_args * actual_args  = (connection_handler_args *)args;
    int sock = actual_args -> socket_desc;
    const char *file_name = actual_args -> file_name;
    LinkedList *paragraphs = actual_args -> paragraphs;

    int read_size;
    char client_message[CHUNK_SIZE];


    if (send_file_to_client(sock, file_name) < 0)
    {
        fprintf(stderr, "Error: sending file failed\n");
    }

    // sending information which paragraphs are locked when client connects
    char* locked_paragraphs_message = create_message_with_lock_status(paragraphs);
    send(sock, locked_paragraphs_message, strlen(locked_paragraphs_message), 0);
    recv(sock, client_message, sizeof(client_message), 0);
    if (strcmp(client_message, "OK") != 0){
        fprintf(stderr, "Error: client response not OK\n");
    }

    add_socket(sock); //adding socket to connected_sockets array

    do
    {
        read_size = recv(sock, client_message, CHUNK_SIZE, 0);
        client_message[read_size] = '\0';

        //when client disconnects
        if(read_size == 0){
            break;
        }

        //reading protocol id
        int protocol_id;
        if(sscanf(client_message, "%d", &protocol_id) == 1) {
//            printf("ProtocolID: %d ", id);
//            fflush(stdout);
        } else {
            fprintf(stderr, "Error: could not parse ID from message\n");
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
            case CHANGE_LINE_VIA_MOUSE_PROTOCOL_ID:
                unlock_paragraph_after_mouse_press(sock, paragraphs, client_message);
                break;
        }

        /* Clear the message buffer */
        memset(client_message, 0, CHUNK_SIZE);
    } while (1);

    unlock_paragraph_with_socket_id(paragraphs, sock);
    fprintf(stderr, "Client disconnected\n");

    // Free the socket pointer
    // TODO: create protocol to unlock paragraph with socket_id when disconnecting
    remove_socket(sock); // remove socket from connected_sockets array
    close(sock);
    pthread_exit(NULL);
}


int send_file_to_client(int sock, const char *file_name)
{
    // Open the file
    FILE *file = fopen(file_name, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error: file not found\n");
        return -1;
    }

    // Turned out that there is not point to seperate sending
    // extension and file name to the client sperately
    // but not deleting the code just in case it could be useful
    // ...............
    // Getting file name and extension
    // char *file_extension = strrchr(file_name, '.'); // strrchr: locate the last occurrence of a character in a string
    // *file_extension = '\0';
    // size_t file_name_size = strlen(file_name) - strlen(file_extension);
    // printf("File name: %s\n", file_extension);
    // char *file_name_wo_extension = (char *)malloc(file_name_size + 1);
    // if (file_name_wo_extension == NULL)
    // {
    // 	fprintf(stderr, "Error: malloc failed\n");
    // 	fclose(file);
    // 	return -1;
    // }
    // file_name_wo_extension = strncpy(file_name_wo_extension, file_name, file_name_size);
    // *file_extension = '.';
    // file_name_wo_extension[file_name_size] = '\0';

    // Send metadata to the client
    char metadata[1024];
    snprintf(metadata, sizeof(metadata), "%d,%d", get_file_size(file), CHUNK_SIZE);
    send(sock, metadata, sizeof(metadata), 0);
    // printf("Metadata: %s\n", metadata);
    // free(file_name_wo_extension);

    // Waiting for confirmation from client
    char client_response[1024];
    recv(sock, client_response, sizeof(client_response), 0);

    if (strcmp(client_response, "OK") != 0)
    {
        fprintf(stderr, "Error: client response not OK\n");
        fclose(file);
        return -1;
    }

    char data_buffer[CHUNK_SIZE];
    int bytes_read = 0;
    // Fread: read data from file then assign to data_buffer
    // return value: number of elements read
    // if return value is 0, it means the end of file
    while ((bytes_read = fread(data_buffer, 1, sizeof(data_buffer), file)) > 0)
    {
        if (send(sock, data_buffer, bytes_read, 0) < 0)
        {
            fprintf(stderr, "Error: send failed\n");
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

int get_file_size(FILE *file)
{
    fseek(file, 0, SEEK_END); // Move the file pointer to the end of the file
    int size = ftell(file);
    fseek(file, 0, SEEK_SET); // Move the file pointer to the beginning of the file
    return size;
}

void update_paragraph_protocol(int sock, LinkedList* paragraphs, char *client_message, int protocol_id){
    // getting paragraph number
    int paragraph_number;
    if(sscanf(client_message, "%d", &paragraph_number) != 1) {
        fprintf(stderr, "Error: could not parse paragraph number from message\n");
        return;
    }
    switch (protocol_id) {
        case SYNC_PROTOCOL_ID:
            lock_paragraph(paragraphs,paragraph_number, sock);
            break;
        case UNLOCK_PARAGRAPH_PROTOCOL_ID:
            unlock_paragraph(paragraphs,paragraph_number, sock);
            break;
    }
    memmove(client_message, client_message + 2, strlen(client_message) - 1);
    edit_content_of_paragraph(paragraphs, paragraph_number, client_message);
    char* paragraph_content = get_content_of_paragraph(paragraphs, paragraph_number);

    // Create the message
    char message[1024];
    snprintf(message, sizeof(message), "%d,%d,%s", protocol_id, paragraph_number, paragraph_content);
    fprintf(stderr, "SocketID: %d, ClientMessage: %s \n", sock, message);
    fflush(stdout);
    refresh_file(paragraphs, FILE_NAME);
    broadcast(message, sock);
}


void async_protocol_new_paragraph(int sock, LinkedList* paragraphs, char* client_message, int insert_after){
    // TODO: extract paragraph number from client message
//    insert_after_node_number(paragraphs, add_after, client_message);
//    refresh_file(paragraphs, FILE_NAME);
//    broadcast(client_message, sock);
}

void async_protocol_delete_paragraph(int sock, LinkedList* paragraphs, int paragraph_number){

}

void add_socket(int socket) {
    pthread_mutex_lock(&connected_sockets_mutex);
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(connected_sockets[i] == -1) {
            connected_sockets[i] = socket;
            break;
        }
    }
    pthread_mutex_unlock(&connected_sockets_mutex);
}

void remove_socket(int socket) {
    pthread_mutex_lock(&connected_sockets_mutex);
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(connected_sockets[i] == socket) {
            connected_sockets[i] = -1;
            break;
        }
    }
    pthread_mutex_unlock(&connected_sockets_mutex);
}

void broadcast(char *message, int sender) {
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(connected_sockets[i] != -1 && connected_sockets[i] != sender) {
            send(connected_sockets[i], message, strlen(message), 0);
        }
    }
}

char* create_message_with_lock_status(LinkedList *paragraphs) {
    Node* temp = paragraphs->head;
    int current_number = 1;
    char* message = malloc(1);
    message[0] = '\0';

    while (temp != NULL) {
        if (temp->locked == 1) {
            char number[10];
            sprintf(number, "%d,", current_number);
            message = realloc(message, strlen(message) + strlen(number) + 1);
            strcat(message, number);
        }
        temp = temp->next;
        current_number++;
    }

    // if it's first client, all paragraphs will be unlocked so return 0
    if(strcmp(message, "") == 0) {
        strcat(message, "0");
    }
    else{
        // adding '0' to indicate end of message if some paragraphs are locked
        message = realloc(message, strlen(message) + 2);
        strcat(message, "0");
    }
    message[strlen(message)] = '\0';
    return message;
}

void unlock_paragraph_after_mouse_press(int sock, LinkedList *paragraphs, char* client_message) {
    int paragraph_number;
    if(sscanf(client_message, "%d", &paragraph_number) != 1) {
        fprintf(stderr, "Error: could not parse paragraph number from message\n");
        return;
    }
    //send paragraph number to all
    char message[1024];
    snprintf(message, sizeof(message), "%d,%d", CHANGE_LINE_VIA_MOUSE_PROTOCOL_ID, paragraph_number);
    unlock_paragraph(paragraphs, paragraph_number, sock);
    broadcast(message, sock);
}
