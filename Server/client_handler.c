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
    char *message, client_message[CHUNK_SIZE];

    add_socket(sock);

    if (send_file_to_client(sock, file_name) < 0)
    {
        fprintf(stderr, "Error: sending file failed\n");
    }

    do
    {
        read_size = recv(sock, client_message, CHUNK_SIZE, 0);
        client_message[read_size] = '\0';

        //when client disconnects
        if(read_size == 0){
            break;
        }

        //reading protocol id
        int id;
        if(sscanf(client_message, "%d", &id) == 1) {
            printf("ProtocolID: %d ", id);
            fflush(stdout);
        } else {
            fprintf(stderr, "Error: could not parse ID from message\n");
        }

        //deleting id from client message
        memmove(client_message, client_message + 2, strlen(client_message) - 1);


        switch (id) {
            case SYNC_PROTOCOL_ID:
                sync_protocol(sock, paragraphs, client_message);
                break;
            case ASYNC_PROTOCOL_ADD_PARAGRAPH:
                async_protocol_new_paragraph(sock, paragraphs, client_message, 1);
                break;
        }


        /* Clear the message buffer */
        memset(client_message, 0, CHUNK_SIZE);
    } while (1);

    fprintf(stderr, "Client disconnected\n");

    // Free the socket pointer
    remove_socket(sock);
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

void sync_protocol(int sock, LinkedList* paragraphs, char *client_message){
    // getting paragraph number
    int paragraph_number;
    if(sscanf(client_message, "%d", &paragraph_number) != 1) {
        fprintf(stderr, "Error: could not parse paragraph number from message\n");
        return;
    }
    fprintf(stderr, "SocketID: %d, ClientMessage: %s \n", sock, client_message);
    memmove(client_message, client_message + 2, strlen(client_message) - 1);
    edit_content_of_paragraph(paragraphs, paragraph_number, client_message);
//    refresh_file(paragraphs, FILE_NAME);
    broadcast(client_message, sock);
}


void async_protocol_new_paragraph(int sock, LinkedList* paragraphs, char* client_message, int insert_after){
    // TODO: extract paragraph number from client message
//    insert_after_node_number(paragraphs, add_after, client_message);
//    refresh_file(paragraphs, FILE_NAME);
//    broadcast(client_message, sock);
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
