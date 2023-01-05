cmake_minimum_required(VERSION 3.18)

##VARIABLES
set(PROJECT_NAME Demo)
set(PROJECT_VERSION 0.0.1)
set(PROJECT_SOURCE_DIR src)
set(PROJECT_SOURCE_MAIN ${PROJECT_SOURCE_DIR}/application.cpp)
set(PROJECT_INCLUDE_DIR include)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

##PRECONDITIONS
if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Use 'cmake -Bbuild' to run it into the build directory")
endif()

##PROJECT
project(
    ${PROJECT_NAME} 
    VERSION ${PROJECT_VERSION}
    LANGUAGES CXX)

add_executable(application ${PROJECT_SOURCE_MAIN})
target_link_libraries(application /usr/local/lib/libusb-1.0.so)
#target_link_libraries(application /home/user/libusb-emscripten/libusb-1.0.so)

#GENERATE version.h
configure_file(version.h.in version.h)
target_include_directories(application PUBLIC "${PROJECT_BINARY_DIR}")
