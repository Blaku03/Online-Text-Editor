#ifndef SERVER_CLIENT_HANDLER_H
#define SERVER_CLIENT_HANDLER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "linked_list.h"

//struct to passing as one argument to the connection handler
typedef struct {
    int socket_desc;
    const char *file_name;
    LinkedList *paragraphs;
} connection_handler_args;

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

#define FILE_NAME "../example_file.txt"
#define PORT 5234
#define MAX_CLIENTS 10
#define CHUNK_SIZE 1024
#define MAX_NUMBER_SKIPPED_CHECK_INS 3
#define SYNC_PROTOCOL_ID 1
#define ASYNC_PROTOCOL_ADD_PARAGRAPH_ID 2
#define ASYNC_PROTOCOL_DELETE_PARAGRAPH_ID 3
#define UNLOCK_PARAGRAPH_PROTOCOL_ID 4
#define CHANGE_LINE_VIA_MOUSE_PROTOCOL_ID 5
extern int connected_sockets[MAX_CLIENTS];
extern pthread_mutex_t connected_sockets_mutex;

void *connection_handler(void *socket_desc);
int send_file_to_client(int sock, const char *file_name);
int get_file_size(FILE *file);
void async_protocol_new_paragraph(int sock, LinkedList* paragraphs, char *client_message, int insert_after);
void async_protocol_delete_paragraph(int sock, LinkedList* paragraphs, int paragraph_number);
void update_paragraph_protocol(int sock, LinkedList* paragraphs, char *client_message, int protocol_id);
void unlock_paragraph_after_mouse_press(int sock, LinkedList* paragraphs, char* client_message);
char* create_message_with_lock_status(LinkedList* paragraphs);
void add_socket(int socket);
void remove_socket(int socket);
void broadcast(char *message, int sender);

#endif //SERVER_CLIENT_HANDLER_H
