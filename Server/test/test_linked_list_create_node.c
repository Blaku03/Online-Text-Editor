#include <stdlib.h>
#include <string.h>

#include "../linked_list.h"

int main(void) {
    // Test case 1
    Node* new_node = linked_list_create_node("");
    if (new_node == NULL) {
        printf("Failed test case 1.1\n");
        return -1;
    }
    if (new_node->locked != 0 || new_node->socket_id != -1 || new_node->next != NULL ||
        new_node->previous != NULL || strcmp(new_node->content, "") != 0) {
        free(new_node);
        printf("Failed test case 1.2\n");
        return -1;
    }
    free(new_node);

    // Test case 2
    new_node = linked_list_create_node("test");
    if (new_node == NULL) {
        printf("Failed test case 2.1\n");
        return -1;
    }
    if (new_node->locked != 0 || new_node->socket_id != -1 || new_node->next != NULL ||
        new_node->previous != NULL || strcmp(new_node->content, "test") != 0) {
        free(new_node);
        printf("Failed test case 2.2\n");
        return -1;
    }
    free(new_node);

    // Test case 3
    new_node = linked_list_create_node(NULL);
    if (new_node != NULL) {
        printf("Failed test case 3\n");
        return -1;
    }

    return 0;
}
