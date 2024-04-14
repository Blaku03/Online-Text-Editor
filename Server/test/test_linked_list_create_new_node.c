#include "../linked_list.h"
#include <string.h>
#include <stdlib.h>

int main(void) {
    Node* new_node = create_new_node("");
    if (new_node == NULL) {
        return -1;
    }
    if (new_node->locked != 0
    || new_node->socket_id != -1
    || new_node->next != NULL
    || new_node->previous != NULL
    || strcmp(new_node->content, "") != 0) {
        free(new_node);
        return -1;
    }
    free(new_node);
    
    new_node = create_new_node("test");
    if (new_node == NULL) {
        return -1;
    }
    if (new_node->locked != 0
    || new_node->socket_id != -1
    || new_node->next != NULL
    || new_node->previous != NULL
    || strcmp(new_node->content, "test") != 0) {
        free(new_node);
        return -1;
    }
    free(new_node);

    new_node = create_new_node(NULL);
    if (new_node != NULL) {
        return -1;
    }
    return 0;
}
