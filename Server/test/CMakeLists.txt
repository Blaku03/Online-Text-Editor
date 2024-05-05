add_compile_options(-g -Wall -Wextra -Wpedantic -Werror)

# linked_list_init
add_executable(test_linked_list_init
        test_linked_list_init.c)
target_link_libraries(test_linked_list_init PRIVATE linked_list)
add_test(NAME test_linked_list_init
        COMMAND test_linked_list_init)

# linked_list_create_node
add_executable(test_linked_list_create_node
        test_linked_list_create_node.c)
target_link_libraries(test_linked_list_create_node PRIVATE linked_list)
add_test(NAME test_linked_list_create_node
        COMMAND test_linked_list_create_node)

# linked_list_insert_before_head
add_executable(test_linked_list_insert_before_head
        test_linked_list_insert_before_head.c)
target_link_libraries(test_linked_list_insert_before_head PRIVATE linked_list)
add_test(NAME test_linked_list_insert_before_head
        COMMAND test_linked_list_insert_before_head)

# linked_list_insert_after_tail
add_executable(test_linked_list_insert_after_tail
        test_linked_list_insert_after_tail.c)
target_link_libraries(test_linked_list_insert_after_tail PRIVATE linked_list)
add_test(NAME test_linked_list_insert_after_tail
        COMMAND test_linked_list_insert_after_tail)

# linked_list_insert_after_node_number
add_executable(test_linked_list_insert_after_node_number
        test_linked_list_insert_after_node_number.c)
target_link_libraries(test_linked_list_insert_after_node_number PRIVATE linked_list)
add_test(NAME test_linked_list_insert_after_node_number
        COMMAND test_linked_list_insert_after_node_number)

# linked_list_get_length
add_executable(test_linked_list_get_length
        test_linked_list_get_length.c)
target_link_libraries(test_linked_list_get_length PRIVATE linked_list)
add_test(NAME test_linked_list_get_length
        COMMAND test_linked_list_get_length)

# linked_list_remove_node
add_executable(test_linked_list_remove_node
        test_linked_list_remove_node.c)
target_link_libraries(test_linked_list_remove_node PRIVATE linked_list)
add_test(NAME test_linked_list_remove_node
        COMMAND test_linked_list_remove_node)
