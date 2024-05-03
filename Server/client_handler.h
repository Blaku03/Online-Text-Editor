#ifndef SERVER_CLIENT_HANDLER_H
#define SERVER_CLIENT_HANDLER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "linked_list.h"

// struct for passing as one argument to the connection handler
typedef struct
{
    int socket_desc;
    const char* file_name;
    LinkedList* paragraphs;
} connection_handler_args;

typedef enum {
    SYNC_PROTOCOL_ID = 1,
    ASYNC_PROTOCOL_ADD_PARAGRAPH_ID,
    ASYNC_PROTOCOL_DELETE_PARAGRAPH_ID,
    UNLOCK_PARAGRAPH_PROTOCOL_ID,
} ProtocolID;

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

#define FILE_NAME                    "../example_file.txt"
#define PORT                         5234
#define MAX_CLIENTS                  10
#define KILOBYTE                     10024
#define CHUNK_SIZE                   KILOBYTE
#define MAX_NUMBER_SKIPPED_CHECK_INS 3
extern int connected_sockets[MAX_CLIENTS];
extern char* user_names[MAX_CLIENTS];
extern pthread_mutex_t connected_sockets_mutex;

void* connection_handler(void* socket_desc);
int send_file_to_client(int sock, const char* file_name);
int get_file_size(FILE* file);
void async_protocol_delete_paragraph(int sock, LinkedList* paragraphs, char* client_message);
void update_paragraph_protocol(int sock, LinkedList* paragraphs, char* client_message, int protocol_id);
int extract_paragraph_number(char* client_message);
char* create_message_with_lock_status(LinkedList* paragraphs);
void delete_id_from_client_message(char* client_message, int id);
void broadcast(char* message, int sender, LinkedList* paragraphs);
const char* get_protocol_name(int protocol_id);
char* get_user_name_by_socket_id(int socket_id);
void modify_socket_array(int search_for, int replace_with);
// Because normal strcmp crashes if one of the strings is NULL
int safe_strcmp(const char* s1, const char* s2);
void modify_username_array(char* search_for, char* replace_with);
int get_number_of_connected_clients();

#endif  // SERVER_CLIENT_HANDLER_H
