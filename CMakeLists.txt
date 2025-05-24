cmake_minimum_required(VERSION 3.10)
project(MapReduceFramework LANGUAGES CXX)

# Use C++20 and pthread
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

# Include directories
include_directories(include)

# Sources for the static library
set(LIB_SOURCES
        src/MapReduceFramework.cpp
        src/JobStateManager.cpp
        src/Barrier.cpp
)

# Create static library
add_library(MapReduceFramework STATIC ${LIB_SOURCES})

target_link_libraries(MapReduceFramework PRIVATE Threads::Threads)

# Sample client executable
add_executable(SampleClient example/SampleClient.cpp)

target_link_libraries(SampleClient PRIVATE MapReduceFramework Threads::Threads)

# Installation and packaging stuff
add_custom_target(tar
        COMMAND ${CMAKE_COMMAND} -E tar "cfv" MapReduceFramework.tar
        ${LIB_SOURCES}
        include/MapReduceClient.h
        include/MapReduceFramework.h
        include/JobStateManager.h
        include/Barrier.h
        ${CMAKE_SOURCE_DIR}/Makefile CMakeLists.txt
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Creating MapReduceFramework.tar"
)

# Set compiler warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(MapReduceFramework PRIVATE -Wall -g)
    target_compile_options(SampleClient PRIVATE -Wall -g)
endif()