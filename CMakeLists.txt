option( BUILD_TESTING "build tests" OFF )
option( BUILD_EXAMPLES "build examples" OFF )

project ( libev C CXX )
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

if(BUILD_TESTING OR BUILD_EXAMPLES)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
endif()

include(CTest)
if(BUILD_TESTING)
  subdirs( test )
  subdirs( bench )
endif()

if(BUILD_EXAMPLES)
  subdirs( examples )
endif()
