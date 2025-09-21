cmake_minimum_required(VERSION 3.0.0)

find_package(PostgreSQL REQUIRED)

add_library(database_manager
    database_manager.cpp
)

target_link_libraries(database_manager PUBLIC
    PostgreSQL::PostgreSQL
    pqxx
)

target_include_directories(database_manager PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_compile_features(database_manager PRIVATE cxx_std_17)

set_target_properties(database_manager PROPERTIES
    CXX_EXTENSIONS OFF
)
