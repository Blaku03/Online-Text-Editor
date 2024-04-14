# init_linked_list
add_executable(test_linked_list_init
        test_linked_list_init.c)
target_compile_options(test_linked_list_init PRIVATE -Wall -Wextra -Werror)
target_link_libraries(test_linked_list_init PRIVATE linked_list)
add_test(NAME test_linked_list_init
        COMMAND test_linked_list_init)

# create_new_node
add_executable(test_linked_list_create_new_node
        test_linked_list_create_new_node.c)
target_compile_options(test_linked_list_create_new_node PRIVATE -Wall -Wextra -Werror)
target_link_libraries(test_linked_list_create_new_node PRIVATE linked_list)
add_test(NAME test_linked_list_create_new_node
        COMMAND test_linked_list_create_new_node)

# insert_after_tail
add_executable(test_linked_list_insert_after_tail
        test_linked_list_insert_after_tail.c)
target_compile_options(test_linked_list_insert_after_tail PRIVATE -Wall -Wextra -Werror)
target_link_libraries(test_linked_list_insert_after_tail PRIVATE linked_list)
add_test(NAME test_linked_list_insert_after_tail
        COMMAND test_linked_list_insert_after_tail)
