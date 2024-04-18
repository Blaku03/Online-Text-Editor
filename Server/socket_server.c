#include "client_handler.h"
#include "linked_list.h"

void start_server(void) {
    LinkedList* paragraphs = (LinkedList*)malloc(sizeof(LinkedList));
    if (paragraphs == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return;
    }
    pthread_mutex_init(&connected_sockets_mutex, NULL);
    linked_list_init(paragraphs);
    parse_file_to_linked_list(paragraphs, FILE_NAME);

    for (int i = 0; i < MAX_CLIENTS; i++) {  // Initialize array of connected_sockets
        connected_sockets[i] = -1;
    }

    int listenfd = 0, connfd = 0;  // listenfd: file descriptor for socket, connfd: file
                                   // descriptor for accept
    sockaddr_in serv_addr;  // sockaddr_in: structure containing an internet address
    pthread_t thread_id;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);  // AF_INET: IPv4, SOCK_STREAM: TCP

    if (listenfd < 0) {
        fprintf(stderr, "Error: socket creation failed\n");
        exit(1);
    }
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr =
        htonl(INADDR_ANY);             // INADDR_ANY: any local machine address
    serv_addr.sin_port = htons(PORT);  // htons: host to network short

    if (bind(listenfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) <
        0) {  // bind the socket to the address and port number specified in addr
        fprintf(stderr, "Error: bind failed\n");
        exit(1);
    }

    if (listen(listenfd, MAX_CLIENTS) <
        0) {  // maximum size of the queue of pending connections
        fprintf(stderr, "Error: listen failed\n");
        exit(1);
    }

    fprintf(stderr, "Server started\n");

    for (;;) {
        if ((connfd = accept(listenfd, (sockaddr*)NULL, NULL)) <
            0) {  // accept a new connection on a socket
            fprintf(stderr, "Error: accept failed\n");
            exit(1);
        }
        fprintf(stderr, "Connection accepted\n");

        connection_handler_args* args = malloc(sizeof(connection_handler_args));
        args->socket_desc = connfd;
        args->file_name = FILE_NAME;
        args->paragraphs = paragraphs;
        pthread_create(&thread_id, NULL, connection_handler, args);
    }

    linked_list_destroy(paragraphs);
}

int main(void) {
    start_server();
}
