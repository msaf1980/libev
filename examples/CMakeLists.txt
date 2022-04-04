add_executable(
    stdin
    stdin.c
)
target_link_libraries( stdin ev )

add_executable(
    stdin_timeout
    stdin_timeout.c
)
target_link_libraries( stdin_timeout ev )

add_executable(
    stdin_no_break
    stdin_no_break.c
)
target_link_libraries( stdin_no_break ev )

add_executable(
    echopp
    echopp.cpp
)
target_link_libraries( echopp ev )
