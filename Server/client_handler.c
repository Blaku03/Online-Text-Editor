#include "client_handler.h"
#include "linked_list.h"
#include <ctype.h>

int connected_sockets[MAX_CLIENTS];
char* user_names[MAX_CLIENTS];
int edit_custom_file = 0;
pthread_mutex_t connected_sockets_mutex;

void* connection_handler(void* args) {
    // casting args
    connection_handler_args* actual_args = (connection_handler_args*)args;
    int sock = actual_args->socket_desc;
    LinkedList* paragraphs = actual_args->paragraphs;
    LinkedList* known_words = actual_args->known_words;

    char client_message[CHUNK_SIZE];

    // TODO: there should be different protocol only
    // for sending number of connected clients but it didn't work
    // when I put it into editor's constructor (if we have time, I can fix it)

    char* file_name = edit_custom_file ? CUSTOM_FILE : FILE_NAME;
    // send_file_to_client returns:
    // 0 if client wants default file
    // 1 if client wants custom file
    // -1 if sending file failed
    switch (send_file_to_client(sock, file_name, paragraphs)) {
    // first client chose default file
    case 0:
        printf("Client has chosen default file\n");
        edit_custom_file = 0;
        break;
    // first client chose custom file
    case 1:
        printf("Client has chosen custom file\n");
        edit_custom_file = 1;
        break;
    // new client connected to opened file
    case 3:
        printf("New client connected to opened file\n");
        break;
    default:
        fprintf(stderr, "Error: sending file failed\n");
        break;
    }

    // sending information which paragraphs are locked when client connects
    char* locked_paragraphs_message = create_message_with_lock_status(paragraphs);
    printf("%s", locked_paragraphs_message);
    send(sock, locked_paragraphs_message, strlen(locked_paragraphs_message), 0);
    free(locked_paragraphs_message);
    locked_paragraphs_message = NULL;

    recv(sock, client_message, sizeof(client_message), 0);
    //    if (strcmp(client_message, "OK") != 0) {
    //        fprintf(stderr, "Error: client response not OK\n");
    //    }
    paragraphs->head->user_name = strdup(client_message);
    modify_username_array(NULL, paragraphs->head->user_name);

    //    add_socket(sock);             //adding socket to connected_sockets array
    modify_socket_array(-1, sock);

    for (;;) {
        unsigned int read_size = recv(sock, client_message, CHUNK_SIZE, 0);
        client_message[read_size] = '\0';

        // when client disconnects
        if (read_size == 0) {
            break;
        }

        // reading protocol id
        int protocol_id = -1;
        if (sscanf(client_message, "%d", &protocol_id) != 1) {
            fprintf(stderr, "Error: could not parse ID from message\n");
            continue;
        }

        delete_id_from_client_message(client_message, protocol_id);

        switch (protocol_id) {
        case SYNC_PROTOCOL_ID:
            update_paragraph_protocol(sock, paragraphs, client_message, SYNC_PROTOCOL_ID);
            break;
        case ASYNC_PROTOCOL_ADD_PARAGRAPH_ID:
            update_paragraph_protocol(sock, paragraphs, client_message, ASYNC_PROTOCOL_ADD_PARAGRAPH_ID);
            break;
        case ASYNC_PROTOCOL_DELETE_PARAGRAPH_ID:
            async_protocol_delete_paragraph(sock, paragraphs, client_message);
            break;
        case UNLOCK_PARAGRAPH_PROTOCOL_ID:
            update_paragraph_protocol(sock, paragraphs, client_message, UNLOCK_PARAGRAPH_PROTOCOL_ID);
            break;
        case ADD_KNOWN_WORD_PROTOCOL_ID:
            add_known_word(sock, known_words, client_message);
            break;
        default:
            fprintf(stderr, "Error: protocol ID is wrong");
            break;
        }
        /* Clear the message buffer */
        memset(client_message, 0, CHUNK_SIZE);
    }

    int unlocked_paragraph = unlock_paragraph_with_socket_id(paragraphs, sock);
    if (unlocked_paragraph != -1) {
        char message[KILOBYTE];
        char dbg_message[KILOBYTE];
        snprintf(
            message, sizeof(message), "%d,%d,%s", UNLOCK_PARAGRAPH_PROTOCOL_ID, unlocked_paragraph + 1, "");
        snprintf(
            dbg_message,
            sizeof(message),
            "%s,%d,%s",
            get_protocol_name(UNLOCK_PARAGRAPH_PROTOCOL_ID),
            unlocked_paragraph + 1,
            "");
        printf("%s", dbg_message);
        broadcast(message, sock, paragraphs);
    }
    fprintf(stderr, "Client disconnected\n");

    // Free the socket pointer
    free(args);
    //    remove_socket(sock);  // remove socket from connected_sockets array
    modify_socket_array(sock, -1);
    modify_username_array(get_user_name_by_socket_id(sock), NULL);
    close(sock);
    pthread_exit(NULL);
}

int send_file_to_client(int sock, const char* file_name, LinkedList* Paragraphs) {
    FILE* file = NULL;

    // Send metadata to the client
    char metadata[KILOBYTE];
    int number_of_connected_clients = get_number_of_connected_clients();
    snprintf(
        metadata,
        sizeof(metadata),
        "%d,%d,%d",
        get_file_size(file_name),
        CHUNK_SIZE,
        number_of_connected_clients);
    send(sock, metadata, sizeof(metadata), 0);

    // Waiting for confirmation from client
    char client_response[KILOBYTE];
    recv(sock, client_response, sizeof(client_response), 0);

    // code for rest of clients
    if (number_of_connected_clients != 0) {
        if (strcmp(client_response, "OK") == 0) {
            char* chosen_filename = edit_custom_file ? CUSTOM_FILE : FILE_NAME;
            file = fopen(chosen_filename, "r");
            if (file == NULL) {
                fprintf(stderr, "Error: file not found\n");
                return -1;
            }
            char data_buffer[CHUNK_SIZE];
            unsigned long bytes_read;
            while ((bytes_read = fread(data_buffer, 1, sizeof(data_buffer), file)) > 0) {
                if (send(sock, data_buffer, bytes_read, 0) < 0) {
                    fprintf(stderr, "Error: send failed\n");
                    fclose(file);
                    return -1;
                }
            }
            fclose(file);
            return 3;
        }
        else {
            fprintf(stderr, "Error: client response not OK\n");
            return -1;
        }
    }

    // code for first client only
    if (strcmp(client_response, "OK") == 0) {
        // first client wants default file
        parse_file_to_linked_list(Paragraphs, file_name);
        file = fopen(file_name, "r");
        if (file == NULL) {
            fprintf(stderr, "Error: file not found\n");
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
    // if first client want custom file
    else if (strcmp(client_response, "SEND_NEW_FILE") == 0) {
        printf("new file detected\n");

        // receiving metadata about custom file from client
        char buf[KILOBYTE];
        recv(sock, buf, sizeof(buf), 0);
        int file_size, chunk_size;
        char* new_filename = NULL;
        if (sscanf(buf, "%d,%d,%s", &file_size, &chunk_size, new_filename) != 2) {
            fprintf(stderr, "Error: could not parse metadata\n");
        }
        else {
            printf(
                "Received metadata: first_integer = %d, second_integer = %d, file_name = %s\n",
                file_size,
                chunk_size,
                new_filename);
        }

        // receiving file from client
        file = fopen(CUSTOM_FILE, "w");
        if (file == NULL) {
            fprintf(stderr, "Error: fileaaaa not found\n");
            return -1;
        }
        char buffer[CHUNK_SIZE];
        int bytes_received;
        while (file_size > 0) {
            bytes_received = recv(sock, buffer, sizeof(buffer), 0);
            if (bytes_received <= 0) {
                break;
            }
            fwrite(buffer, 1, bytes_received, file);
            file_size -= bytes_received;
        }

        fclose(file);

        // parse file to linked list
        parse_file_to_linked_list(Paragraphs, CUSTOM_FILE);
        refresh_file(Paragraphs, CUSTOM_FILE);

        return 1;
    }

    fprintf(stderr, "Error: client response not OK\n");
    fclose(file);
    return -1;
}

int get_file_size(const char* filename) {
    FILE* file = fopen(filename, "r");
    fseek(file, 0, SEEK_END);  // Move the file pointer to the end of the file
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);  // Move the file pointer to the beginning of the file
    fclose(file);
    return size;
}

void update_paragraph_protocol(int sock, LinkedList* paragraphs, char* client_message, int protocol_id) {
    int paragraph_number = extract_paragraph_number(client_message);

    char* paragraph_content = NULL;

    switch (protocol_id) {
    case SYNC_PROTOCOL_ID:
        lock_paragraph(paragraphs, paragraph_number, sock, get_user_name_by_socket_id(sock));
        paragraph_content = edit_content_of_paragraph(paragraphs, paragraph_number, client_message);
        break;
    case UNLOCK_PARAGRAPH_PROTOCOL_ID:
        unlock_paragraph(paragraphs, paragraph_number, sock);
        paragraph_content = get_content_of_paragraph(paragraphs, paragraph_number);
        break;
    case ASYNC_PROTOCOL_ADD_PARAGRAPH_ID:
        unlock_paragraph(paragraphs, paragraph_number, sock);
        linked_list_insert_after_node_number(paragraphs, paragraph_number, linked_list_create_node("\n"));
        paragraph_content = edit_content_of_paragraph(paragraphs, paragraph_number, client_message);
        lock_paragraph(paragraphs, paragraph_number + 1, sock, get_user_name_by_socket_id(sock));
        break;
    }

    // remove \n from paragraph content

    // Create the message
    char message[KILOBYTE];
    char dbg_message[KILOBYTE];
    snprintf(message, sizeof(message), "%d,%d,%s", protocol_id, paragraph_number, paragraph_content);
    snprintf(
        dbg_message,
        sizeof(message),
        "%s,%d,%s",
        get_protocol_name(protocol_id),
        paragraph_number,
        paragraph_content);
    fprintf(stderr, "SocketID: %d, ClientMessage: %s \n", sock, dbg_message);
    fflush(stdout);

    if (edit_custom_file)
        refresh_file(paragraphs, CUSTOM_FILE);
    else
        refresh_file(paragraphs, FILE_NAME);

    broadcast(message, sock, paragraphs);
}

int extract_paragraph_number(char* client_message) {
    int paragraph_number;
    if (sscanf(client_message, "%d", &paragraph_number) != 1) {
        fprintf(stderr, "Error: could not parse paragraph number from message\n");
    }

    // deleting paragraph number based on its length
    char paragraph_number_as_string[20];
    sprintf(paragraph_number_as_string, "%d", paragraph_number);
    unsigned int digits_of_paragraph_number = strlen(paragraph_number_as_string);
    memmove(
        client_message,
        client_message + digits_of_paragraph_number + 1,
        strlen(client_message) - digits_of_paragraph_number);

    return paragraph_number;
}

void delete_id_from_client_message(char* client_message, int id) {
    // deleting id based on its length
    char id_as_string[20];
    sprintf(id_as_string, "%d", id);
    unsigned int digits_of_paragraph_number = strlen(id_as_string);
    memmove(
        client_message,
        client_message + digits_of_paragraph_number + 1,
        strlen(client_message) - digits_of_paragraph_number);
}

void async_protocol_delete_paragraph(int sock, LinkedList* paragraphs, char* client_message) {
    int paragraph_number = extract_paragraph_number(client_message);
    char* content_of_paragraph_to_delete = get_content_of_paragraph(paragraphs, paragraph_number);
    char* content_of_paragraph_above = get_content_of_paragraph(paragraphs, paragraph_number - 1);
    content_of_paragraph_above[strcspn(content_of_paragraph_above, "\n")] = '\0';  // trim '\n'

    size_t new_size = strlen(content_of_paragraph_above) + strlen(content_of_paragraph_to_delete) + 1;
    char* reallocated_content = realloc(content_of_paragraph_above, new_size);
    if (reallocated_content == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return;
    }
    content_of_paragraph_above = reallocated_content;
    strcat(content_of_paragraph_above, content_of_paragraph_to_delete);

    edit_content_of_paragraph(paragraphs, paragraph_number - 1, content_of_paragraph_above);
    linked_list_destroy_node(linked_list_remove_node(paragraphs, paragraph_number));

    // Create the message
    char message[KILOBYTE];
    snprintf(message, sizeof(message), "%d,%d", ASYNC_PROTOCOL_DELETE_PARAGRAPH_ID, paragraph_number);
    fprintf(stderr, "SocketID: %d, ClientMessage: %s \n", sock, message);
    fflush(stdout);

    if (edit_custom_file)
        refresh_file(paragraphs, CUSTOM_FILE);
    else
        refresh_file(paragraphs, FILE_NAME);
    broadcast(message, sock, paragraphs);
}

int safe_strcmp(const char* s1, const char* s2) {
    if (s1 == NULL) {
        return s2 == NULL ? 0 : 1;
    }
    if (s2 == NULL) {
        return 1;
    }
    return strcmp(s1, s2);
}

void modify_username_array(char* search_for, char* replace_with) {
    pthread_mutex_lock(&connected_sockets_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (safe_strcmp(user_names[i], search_for) == 0) {
            if (replace_with == NULL) {
                free(user_names[i]);
            }
            user_names[i] = replace_with;
            break;
        }
    }
    pthread_mutex_unlock(&connected_sockets_mutex);
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

void broadcast(char* message, int sender, LinkedList* paragraphs) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (connected_sockets[i] != -1 && connected_sockets[i] != sender) {
            send(connected_sockets[i], message, strlen(message), 0);
            usleep(1000);
            char* lock_status_message = create_message_with_lock_status(paragraphs);
            send(connected_sockets[i], lock_status_message, strlen(lock_status_message), 0);
            free(lock_status_message);
        }
    }
}

int get_number_of_connected_clients() {
    int number_of_connected_clients = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (connected_sockets[i] != -1) {
            number_of_connected_clients++;
        }
    }
    return number_of_connected_clients;
}

char* create_message_with_lock_status(LinkedList* paragraphs) {
    Node* temp = paragraphs->head;
    int current_number_itr = 1;
    int total_space_to_allocate = 0;

    // count the number of locked paragraphs
    while (temp != NULL) {
        if (temp->locked == 1) {
            total_space_to_allocate++;
            total_space_to_allocate += sizeof(temp->user_name);
        }
        temp = temp->next;
    }
    temp = paragraphs->head;

    // Allocate memory for the message
    // Each number will be at most 10 characters long, plus one for the comma
    char* message = malloc(total_space_to_allocate + 2);  // +2 for the "0" and the null terminator
    if (message == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return NULL;
    }

    message[0] = '\0';
    while (temp != NULL) {
        if (temp->locked == 1) {
            char mess_buffer[KILOBYTE];
            sprintf(mess_buffer, "%d %s,", current_number_itr, temp->user_name);
            strcat(message, mess_buffer);
        }
        temp = temp->next;
        current_number_itr++;
    }

    strcat(message, "0");  // Add a zero to indicate the end of the message
    return message;
}

char* get_user_name_by_socket_id(int socket_id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (connected_sockets[i] == socket_id) {
            return user_names[i];
        }
    }
    return NULL;
}

const char* get_protocol_name(int protocol_id) {
    switch (protocol_id) {
    case SYNC_PROTOCOL_ID:
        return "SYNC PROTOCOL";
    case ASYNC_PROTOCOL_ADD_PARAGRAPH_ID:
        return "ADD PARAGRAPH";
    case ASYNC_PROTOCOL_DELETE_PARAGRAPH_ID:
        return "DELETE PARAGRAPH";
    case UNLOCK_PARAGRAPH_PROTOCOL_ID:
        return "UNLOCK PARAGRAPH";
    case ADD_KNOWN_WORD_PROTOCOL_ID:
        return "ADD KNOWN WORD";
    default:
        return "UNKNOWN PROTOCOL";
    }
}

void add_known_word(int sock, LinkedList* known_words, char* client_message) {
    // strip whitespace at the start
    while (isspace(*client_message) && *client_message != '\0') {
        client_message++;
    }

    // strip whitespace at the end
    char* end = client_message;
    while (!isspace(*end) && *end != '\0') {
        end++;
    }
    *end = '\0';

    // return early if there aren't any non-whitespace characters in the word
    if (client_message == end) {
        return;
    }

    // return early if node creation fails
    if (linked_list_insert_before_head(known_words, linked_list_create_node(client_message)) == -1) {
        return;
    }

    // broadcast new word to other clients
    char message[KILOBYTE];
    int message_length =
        snprintf(message, sizeof(message), "%d,%s,", ADD_KNOWN_WORD_PROTOCOL_ID, client_message);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (connected_sockets[i] != -1) {
            send(connected_sockets[i], message, message_length, 0);
        }
    }
    printf("%s\n", message);
}