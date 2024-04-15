#include "../linked_list.h"

int main(void) {
    // Test case 1
    LinkedList list;
    linked_list_init(&list);
    if (list.head != NULL || list.tail != NULL) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        printf("Failed test case 1.1\n");
        return -1;
    }
    if (pthread_mutex_trylock(&list.linked_list_mutex) != 0) {
        printf("Failed test case 1.2\n");
        return -1;
    }
    pthread_mutex_unlock(&list.linked_list_mutex);
    pthread_mutex_destroy(&list.linked_list_mutex);

    // Test case 2
    // This should crash on failure.
    linked_list_init(NULL);

    return 0;
}
