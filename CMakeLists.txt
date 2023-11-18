cmake_minimum_required(VERSION 3.5.0)
project(distributed_memory VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

find_package(MPI REQUIRED)
add_definitions(-DOMPI_SKIP_MPICXX)

include_directories(SYSTEM ${MPI_INCLUDE_PATH})

add_executable(distributed_memory hkr33_distributed_memory.c)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -Wextra -Wconversion")

target_link_libraries(distributed_memory ${MPI_C_LIBRARIES})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)