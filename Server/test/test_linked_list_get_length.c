#include "../linked_list.h"

Node create_dummy_node() {
    Node dummy;
    dummy.content = NULL;
    dummy.locked = 0;
    dummy.socket_id = -1;
    dummy.next = NULL;
    dummy.previous = NULL;
    return dummy;
}

int main(void) {
    // Test case 1
    if (linked_list_get_length(NULL) != -1) {
        printf("Failed test case 1\n");
        return -1;
    }
    LinkedList list;
    list.head = NULL;
    list.tail = NULL;
    pthread_mutex_init(&list.linked_list_mutex, NULL);
    Node first_node = create_dummy_node();
    Node second_node = create_dummy_node();
    Node third_node = create_dummy_node();

    // Test case 2
    if (linked_list_get_length(&list) != 0) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 2\n");
        return -1;
    }
    
    list.head = &first_node;
    list.tail = &first_node;

    // Test case 3
    if (linked_list_get_length(&list) != 1) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 3\n");
        return -1;
    }

    first_node.next = &second_node;
    second_node.previous = &first_node;
    list.tail = &second_node;
    
    // Test case 4
    if (linked_list_get_length(&list) != 2) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 4\n");
        return -1;
    }

    second_node.next = &third_node;
    third_node.previous = &second_node;
    list.tail = &third_node;

    // Test case 5
    if (linked_list_get_length(&list) != 3) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 5\n");
        return -1;
    }

    pthread_mutex_destroy(&list.linked_list_mutex);
    return 0;
}
