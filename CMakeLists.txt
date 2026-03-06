cmake_minimum_required(VERSION 3.0.0)
project(nodepp-postgres VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(NODEPP_POSTGRES_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Search for Nodepp Core
if (NOT TARGET nodepp)
    find_package(nodepp REQUIRED)
    if(NOT nodepp_FOUND)
        message(FATAL_ERROR "Nodepp not found. Please install nodepp.")
    endif()
endif()

# 1. Search for PostgreSQL
find_package(PostgreSQL REQUIRED) 
if(NOT POSTGRESQL_FOUND)
    message(FATAL_ERROR "PostgreSQL client library (libpq) not found. Please install libpq-dev or equivalent files.")
endif()

# interface-target
add_library(nodepp-postgres INTERFACE)

# Combine all necessary include directories
target_include_directories(nodepp-postgres INTERFACE
    ${NODEPP_POSTGRES_INCLUDE_DIR}
    ${POSTGRESQL_INCLUDE_DIRS}
)

# Link against all dependencies
target_link_libraries(nodepp-postgres INTERFACE
    nodepp 
    ${POSTGRESQL_LIBRARIES}
)