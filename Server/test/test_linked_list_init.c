#include "../linked_list.h"

int main(void) {
    LinkedList list;
    init_linked_list(&list);
    if (list.head != NULL || list.tail != NULL) {
        pthread_mutex_destroy(&list.linked_list_mutex);
        return -1;
    }
    if (pthread_mutex_trylock(&list.linked_list_mutex) != 0) {
        return -1;
    }
    pthread_mutex_unlock(&list.linked_list_mutex);
    pthread_mutex_destroy(&list.linked_list_mutex);
    return 0;
}
