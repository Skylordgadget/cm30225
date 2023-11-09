cmake_minimum_required(VERSION 3.5.0)
project(cwk1 VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

include_directories(inc)

# add_executable(cwk1 wikipediea.c)
# add_executable(cwk1 frieden.c)
# add_executable(cwk1 hkr33_shared_memory.c)
add_executable(cwk1 hkr33_shared_memory_sequential.c)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -Wextra -Wconversion")
 
set(THREADS_PREFER_PTHREAD_FLAG ON)
 
find_package(Threads REQUIRED)

target_link_libraries(cwk1 PRIVATE Threads::Threads)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)