cmake_minimum_required(VERSION 3.13)

project(HTTP-server)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp include/*.hpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(server ${SOURCE_FILES})

target_include_directories(server PRIVATE include)

target_link_libraries(server PRIVATE Threads::Threads ZLIB::ZLIB)
