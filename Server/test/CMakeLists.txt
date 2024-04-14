add_executable(test_linked_list_init
        test_linked_list_init.c)

target_link_libraries(test_linked_list_init PRIVATE linked_list)

add_test(NAME test_linked_list_init
        COMMAND test_linked_list_init)
