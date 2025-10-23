cmake_minimum_required(VERSION 3.12)
project(searcher)

find_package(Boost REQUIRED COMPONENTS system)

# add_subdirectory(../database_manager ${CMAKE_BINARY_DIR}/database_manager)

add_executable(searcher
    main.cpp
    http_server.cpp
)

target_link_libraries(searcher
    Boost::system
    database_manager
    pthread
)

target_compile_features(searcher PRIVATE cxx_std_11)
