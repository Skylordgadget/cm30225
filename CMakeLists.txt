cmake_minimum_required(VERSION 3.5.0)
project(distributed_memory VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

include_directories(inc)

# add_executable(distributed_memory wikipediea.c)
# add_executable(distributed_memory frieden.c)
add_executable(distributed_memory hkr33_distributed_memory.c)
# add_executable(distributed_memory hkr33_distributed_memory_sequential.c)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -Wextra -Wconversion")
 
set(THREADS_PREFER_PTHREAD_FLAG ON)
 
find_package(Threads REQUIRED)

target_link_libraries(distributed_memory PRIVATE Threads::Threads)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)