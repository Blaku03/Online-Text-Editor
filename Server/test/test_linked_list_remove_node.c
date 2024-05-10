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

void link_nodes(Node* first, Node* second) {
    first->next = second;
    second->previous = first;
}

int main(void) {
    // Test case 1
    if (linked_list_remove_node(NULL, 1) != NULL) {
        printf("Failed test case 1\n");
        return -1;
    }
    Node first_node = create_dummy_node();
    Node second_node = create_dummy_node();
    Node third_node = create_dummy_node();
    link_nodes(&first_node, &second_node);
    link_nodes(&second_node, &third_node);
    LinkedList list;
    list.head = &first_node;
    list.tail = &third_node;
    pthread_mutex_init(&list.linked_list_mutex, NULL);

    // Test case 2
    if (linked_list_remove_node(&list, 3) != &third_node) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 2.1\n");
        return -1;
    }
    if (list.tail != &second_node || list.tail->previous != &first_node || list.tail->next != NULL) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 2.2\n");
        return -1;
    }

    first_node = create_dummy_node();
    second_node = create_dummy_node();
    third_node = create_dummy_node();
    link_nodes(&first_node, &second_node);
    link_nodes(&second_node, &third_node);
    list.head = &first_node;
    list.tail = &third_node;

    // Test case 3
    if (linked_list_remove_node(&list, 2) != &second_node) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 3.1\n");
        return -1;
    }
    if (list.head != &first_node || list.tail != &third_node || list.head->next != &third_node ||
        list.tail->previous != &first_node) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 3.2\n");
        return -1;
    }

    first_node = create_dummy_node();
    second_node = create_dummy_node();
    third_node = create_dummy_node();
    link_nodes(&first_node, &second_node);
    link_nodes(&second_node, &third_node);
    list.head = &first_node;
    list.tail = &third_node;

    // Test case 4
    if (linked_list_remove_node(&list, 1) != &first_node) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 4.1\n");
        return -1;
    }
    if (list.head != &second_node || list.head->previous != NULL || list.head->next != &third_node) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 4.2\n");
        return -1;
    }

    first_node = create_dummy_node();
    second_node = create_dummy_node();
    third_node = create_dummy_node();
    link_nodes(&first_node, &second_node);
    link_nodes(&second_node, &third_node);
    list.head = &first_node;
    list.tail = &third_node;

    // Test case 5
    if (linked_list_remove_node(&list, 0) != &first_node) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 5.1\n");
        return -1;
    }
    if (list.head != &second_node || list.head->previous != NULL || list.head->next != &third_node) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 5.2\n");
        return -1;
    }

    first_node = create_dummy_node();
    list.head = &first_node;
    list.tail = &first_node;

    // Test case 6
    if (linked_list_remove_node(&list, 1) != &first_node) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 6.1\n");
        return -1;
    }
    if (list.head != NULL || list.tail != NULL) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 6.2\n");
        return -1;
    }

    first_node = create_dummy_node();
    list.head = &first_node;
    list.tail = &first_node;

    // Test case 7
    if (linked_list_remove_node(&list, 0) != &first_node) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 7.1\n");
        return -1;
    }
    if (list.head != NULL || list.tail != NULL) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 7.2\n");
        return -1;
    }

    list.head = NULL;
    list.tail = NULL;

    // Test case 8
    if (linked_list_remove_node(&list, 1) != NULL) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 8\n");
        return -1;
    }

    list.head = NULL;
    list.tail = NULL;

    // Test case 9
    if (linked_list_remove_node(&list, 0) != NULL) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 9\n");
        return -1;
    }

    pthread_mutex_destroy(&list.linked_list_mutex);
    return 0;
}
