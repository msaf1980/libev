project ( libev C )
cmake_minimum_required ( VERSION 3.0 )
include ( CMake/dist.cmake )
include ( CMake/configure.cmake )

configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set ( EV_SRC
  ev.c
  event.c
)

if (WIN32)
  list ( APPEND EV_LIBS Ws2_32 )
endif ()

add_library ( ev ${EV_SRC} )
target_link_libraries ( ev ${EV_LIBS} )

install_library ( ev )
install_header ( ev.h ev++.h event.h )
install_data ( LICENSE README )
