#include <pthread.h>
#include <signal.h>
#include "client_handler.h"
#include "linked_list.h"

void add_thread(pthread_t thread_id, int socket){
    pthread_mutex_lock(&active_threads_mutex);
    for(int i = 0; i < MAX_CLIENTS; i++){
        if(active_threads[i].is_empty == 1){
            active_threads[i].is_empty = 0;
            active_threads[i].thread_id = thread_id;
            active_threads[i].socket_id = socket;
            active_threads[i].is_checked = 1;
            break;
        }
    }
    pthread_mutex_unlock(&active_threads_mutex);
}

void* activity_check_routine(void *arg){
    LinkedList* paragraphs = (LinkedList*)arg;
    for(;;){
        sleep(TIMEOUT_SECONDS);
        pthread_mutex_lock(&active_threads_mutex);
        for(int i = 0; i < MAX_CLIENTS; i++){
            if(active_threads[i].is_empty == 0){
                if(active_threads[i].is_checked == 0){
                    fprintf(stderr, "Client with socket id %d is inactive\n", active_threads[i].socket_id);
                    update_paragraph_protocol(active_threads[i].socket_id, paragraphs, "message", UNLOCK_PARAGRAPH_PROTOCOL_ID);
                    connected_sockets[i] = -1;
                    active_threads[i].is_checked = -1;
                    active_threads[i].socket_id = -1;
                    active_threads[i].thread_id = -1;
                    active_threads[i].is_empty = 1;
                }
                else if(active_threads[i].is_checked == 1){
                    active_threads[i].is_checked = 0;
                }
            }
        }
        pthread_mutex_unlock(&active_threads_mutex);
        printf("test\n");
    }
}

void start_server(void) {
    LinkedList* paragraphs = (LinkedList*)malloc(sizeof(LinkedList));
    LinkedList* known_words = (LinkedList*)malloc(sizeof(LinkedList));
    if (paragraphs == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return;
    }
    pthread_mutex_init(&connected_sockets_mutex, NULL);
    linked_list_init(paragraphs);
    linked_list_init(known_words);

    for (int i = 0; i < MAX_CLIENTS; i++) {  // Initialize array of connected_sockets
        connected_sockets[i] = -1;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {  // Initialize array of active_threads
        active_threads[i].is_checked = -1;
        active_threads[i].socket_id = -1;
        active_threads[i].thread_id = -1;
        active_threads[i].is_empty = 1;
    }

    int listenfd = 0, connfd = 0;  // listenfd: file descriptor for socket, connfd: file
                                   // descriptor for accept
    sockaddr_in serv_addr;         // sockaddr_in: structure containing an internet address
    pthread_t thread_id;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);  // AF_INET: IPv4, SOCK_STREAM: TCP

    if (listenfd < 0) {
        fprintf(stderr, "Error: socket creation failed\n");
        exit(1);
    }
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // INADDR_ANY: any local machine address
    serv_addr.sin_port = htons(PORT);               // htons: host to network short

//    struct timeval timeout = {TIMEOUT_SECONDS, 0};
//
//    // Set the option for the socket
//    if (setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
//        fprintf(stderr, "Error: setsockopt failed\n");
//        return;
//    }

    if (bind(listenfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) <
        0) {  // bind the socket to the address and port number specified in addr
        fprintf(stderr, "Error: bind failed\n");
        exit(1);
    }

    if (listen(listenfd, MAX_CLIENTS) < 0) {  // maximum size of the queue of pending connections
        fprintf(stderr, "Error: listen failed\n");
        exit(1);
    }

    fprintf(stderr, "Server started\n");

    // creating new thread for checking activity
    pthread_create(&thread_id, NULL, activity_check_routine, paragraphs);

    for (;;) {
        if ((connfd = accept(listenfd, (sockaddr*)NULL, NULL)) < 0) {  // accept a new connection on a socket
            fprintf(stderr, "Error: accept failed\n");
            exit(1);
        }
        fprintf(stderr, "Connection accepted\n");

        connection_handler_args* args = malloc(sizeof(connection_handler_args));
        args->socket_desc = connfd;
        args->paragraphs = paragraphs;
        args->known_words = known_words;
        pthread_create(&thread_id, NULL, connection_handler, args);
        add_thread(thread_id, connfd);
    }
    linked_list_destroy(paragraphs);
}


int main(void) {
    start_server();
}
