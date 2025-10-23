cmake_minimum_required(VERSION 3.12)
project(SearchEngine)

find_package(Boost REQUIRED COMPONENTS system)

add_subdirectory(../database_manager ${CMAKE_BINARY_DIR}/database_manager)

add_executable(search_engine
    main.cpp
    http_server.cpp
)

# target_include_directories(${PROJECT_NAME} PUBLIC
#     ${CMAKE_SOURCE_DIR}/
# )

target_link_libraries(search_engine
    Boost::system
    database_manager
    pthread
)

target_compile_features(search_engine PRIVATE cxx_std_11)
