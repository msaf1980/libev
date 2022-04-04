add_executable(
    bench
    bench.c
)
target_link_libraries( bench ev )
target_include_directories( bench PRIVATE ${PROJECT_SOURCE_DIR} )
