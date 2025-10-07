cmake_minimum_required(VERSION 3.0.0)

add_library(indexer
    indexer.cpp
)

target_include_directories(indexer PUBLIC
    ${PROJECT_SOURCE_DIR}/
)

target_link_libraries(indexer PRIVATE
    utils
    parser
    database_manager
)
