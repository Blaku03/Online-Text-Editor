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
#define MAX_CLIENTS 10
#define CHUNK_SIZE 1024
#define MAX_NUMBER_SKIPPED_CHECK_INS 3

int get_file_size(FILE *file)
{
  fseek(file, 0, SEEK_END); // Move the file pointer to the end of the file
  int size = ftell(file);
  fseek(file, 0, SEEK_SET); // Move the file pointer to the beginning of the file
  return size;
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

void *connection_handler(void *socket_desc)
{
  int sock = *(int *)socket_desc; // Get the socket descriptor
  int read_size;
  char *message, client_message[CHUNK_SIZE];
  char file_name[] = "example_file.txt";

  if (send_file_to_client(sock, file_name) < 0)
  {
    fprintf(stderr, "Error: sending file failed\n");
  }

  do
  {
    read_size = recv(sock, client_message, CHUNK_SIZE, 0); // Receive a message from client
    client_message[read_size] = '\0';

    /* Clear the message buffer */
    memset(client_message, 0, CHUNK_SIZE);
  } while (read_size > 2); /* Wait for empty line */

  fprintf(stderr, "Client disconnected\n");

  // Free the socket pointer
  close(sock);
  pthread_exit(NULL);
}

void start_server(void)
{
  int listenfd = 0, connfd = 0; // listenfd: file descriptor for socket, connfd: file descriptor for accept
  sockaddr_in serv_addr;        // sockaddr_in: structure containing an internet address
  pthread_t thread_id;

  listenfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET: IPv4, SOCK_STREAM: TCP
  if (listenfd < 0)
  {
    fprintf(stderr, "Error: socket creation failed\n");
    exit(1);
  }
  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY: any local machine address
  serv_addr.sin_port = htons(PORT);              // htons: host to network short

  if (bind(listenfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) // bind the socket to the address and port number specified in addr
  {
    fprintf(stderr, "Error: bind failed\n");
    exit(1);
  }

  if (listen(listenfd, MAX_CLIENTS) < 0) // maximum size of the queue of pending connections
  {
    fprintf(stderr, "Error: listen failed\n");
    exit(1);
  }

  fprintf(stderr, "Server started\n");

  for (;;)
  {
    if ((connfd = accept(listenfd, (sockaddr *)NULL, NULL)) < 0) // accept a new connection on a socket
    {
      fprintf(stderr, "Error: accept failed\n");
      exit(1);
    }
    fprintf(stderr, "Connection accepted\n");
    pthread_create(&thread_id, NULL, connection_handler, (void *)&connfd);
  }
}

int main(int argc, char *argv[])
{
  start_server();
}
