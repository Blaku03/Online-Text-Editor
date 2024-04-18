#include "../linked_list.h"
#include <pthread.h>

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
    LinkedList list;
    list.head = NULL;
    list.tail = NULL;
    pthread_mutex_init(&list.linked_list_mutex, NULL);
    Node first_node = create_dummy_node();
    Node second_node = create_dummy_node();
    Node third_node = create_dummy_node();

    // Test case 1
    if (linked_list_insert_before_head(&list, &second_node) != 0) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 1.1\n");
        return -1;
    }
    if (list.head != &second_node || list.tail != &second_node ||
        list.tail->next != NULL || list.tail->previous != NULL) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 1.2\n");
        return -1;
    }

    // Test case 2
    if (linked_list_insert_before_head(&list, &first_node) != 0) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 2.1\n");
        return -1;
    }
    if (list.head != &first_node || list.tail != &second_node ||
        list.head->next != &second_node || list.head->previous != NULL ||
        list.tail->next != NULL || list.tail->previous != &first_node) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 2.2\n");
        return -1;
    }

    // Test case 3
    if (linked_list_insert_before_head(&list, NULL) == 0) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 3.1\n");
        return -1;
    }
    if (list.head != &first_node || list.head->previous != NULL) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 3.2\n");
        return -1;
    }

    // Test case 4
    if (linked_list_insert_before_head(NULL, &third_node) == 0) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 4.1\n");
        return -1;
    }
    if (third_node.next != NULL || third_node.previous != NULL) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 4.2\n");
        return -1;
    }

    // Test case 5
    if (linked_list_insert_before_head(NULL, NULL) == 0) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 5\n");
        return -1;
    }

    pthread_mutex_destroy(&list.linked_list_mutex);
    return 0;
}
