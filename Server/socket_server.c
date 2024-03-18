#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

#define PORT 5234

void *connection_handler(void *socket_desc)
{
	int sock = *(int *)socket_desc; // Get the socket descriptor
	int read_size;
	char *message, client_message[2000];

	do
	{
		read_size = recv(sock, client_message, 2000, 0); // Receive a message from client
		client_message[read_size] = '\0';

		/* Send the message back to client */
		 write(sock, client_message, strlen(client_message));
		printf("Client message: %s\n", client_message);

		/* Clear the message buffer */
		memset(client_message, 0, 2000);
	} while (read_size > 2); /* Wait for empty line */

	fprintf(stderr, "Client disconnected\n");

	// Free the socket pointer
	close(sock);
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	int listenfd = 0, connfd = 0; // listenfd: file descriptor for socket, connfd: file descriptor for accept
	sockaddr_in serv_addr;				// sockaddr_in: structure containing an internet address

	pthread_t thread_id;

	listenfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET: IPv4, SOCK_STREAM: TCP
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY: any local machine address
	serv_addr.sin_port = htons(PORT);							 // htons: host to network short

	bind(listenfd, (sockaddr *)&serv_addr, sizeof(serv_addr)); // bind the socket to the address and port number specified in addr

	listen(listenfd, 10); // 10 is the maximum size of the queue of pending connections

	for (;;)
	{
		connfd = accept(listenfd, (sockaddr *)NULL, NULL); // accept a new connection on a socket
		fprintf(stderr, "Connection accepted\n");
		pthread_create(&thread_id, NULL, connection_handler, (void *)&connfd);
	}
}
