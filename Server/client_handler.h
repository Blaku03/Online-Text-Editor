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
extern int connected_sockets[MAX_CLIENTS];
extern pthread_mutex_t connected_sockets_mutex;

void *connection_handler(void *socket_desc);
int send_file_to_client(int sock, const char *file_name);
int get_file_size(FILE *file);
void sync_protocol(int sock, LinkedList* paragraphs, char *client_message);
void add_socket(int socket);
void remove_socket(int socket);
void broadcast(char *message, int sender);

#endif //SERVER_CLIENT_HANDLER_H