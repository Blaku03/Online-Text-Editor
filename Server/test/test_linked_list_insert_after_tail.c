#include "../linked_list.h"

int main(void) {
    LinkedList list;
    list.head = NULL;
    list.tail = NULL;
    pthread_mutex_init(&list.linked_list_mutex, NULL);
    Node first_node;
    first_node.content = NULL;
    first_node.locked = 0;
    first_node.socket_id = -1;
    first_node.next = NULL;
    first_node.previous = NULL;
    Node second_node;
    second_node.content = NULL;
    second_node.locked = 0;
    second_node.socket_id = -1;
    second_node.next = NULL;
    second_node.previous = NULL;

    insert_after_tail(&list, &first_node);
    if (list.head != &first_node
    || list.tail != &first_node
    || list.tail->next != NULL
    || list.tail->previous != NULL) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        return -1;
    }

    insert_after_tail(&list, &second_node);
    if (list.head != &first_node
    || list.tail != &second_node
    || list.head->next != &second_node
    || list.head->previous != NULL
    || list.tail->next != NULL
    || list.tail->previous != &first_node) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        return -1;
    }

    pthread_mutex_destroy(&list.linked_list_mutex);
    return 0;
}
