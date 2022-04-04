set ( TEST_LIBS ev )

include_directories( ${PROJECT_SOURCE_DIR} )

add_executable(
    test_init
    init_test.c
)
target_link_libraries( test_init ev ${TEST_LIBS} )
add_test( NAME test_init COMMAND test_init )

add_executable(
    test_signal
    signal_test.c
)
target_link_libraries( test_signal ev ${TEST_LIBS} )
add_test( NAME test_signal COMMAND test_signal )

add_executable(
    test_child
    child_test.c
)
target_link_libraries( test_child ev ${TEST_LIBS} )
add_test( NAME test_child COMMAND test_child )

add_executable(
    test_async_notify
    async_notify_test.c
)
target_link_libraries( test_async_notify ev ${TEST_LIBS} Threads::Threads )
add_test( NAME test_async_notify COMMAND test_async_notify )

add_executable(
    test_timer
    timer_test.c
)
target_link_libraries( test_timer ev ${TEST_LIBS} )
add_test( NAME test_timer COMMAND test_timer )

add_executable(
    test_reof
    reof_test.c
)
target_link_libraries( test_reof ev ${TEST_LIBS} )
add_test( NAME test_reof COMMAND test_reof )

add_executable(
    test_weof
    weof_test.c
)
target_link_libraries( test_weof ev ${TEST_LIBS} )
add_test( NAME test_weof COMMAND test_weof )

add_executable(
    test_timeout
    timeout_test.c
)
target_link_libraries( test_timeout ev ${TEST_LIBS} )
add_test( NAME test_timeout COMMAND test_timeout )
