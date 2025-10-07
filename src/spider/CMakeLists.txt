cmake_minimum_required(VERSION 3.0.0)

add_subdirectory(page_loader)
add_subdirectory(parser)

add_library(spider
    spider.cpp
    indexer/indexer.cpp
)

target_include_directories(spider PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(spider PUBLIC
    database_manager
    utils
    page_loader
    parser
)

target_compile_features(spider PUBLIC cxx_std_17)

set_target_properties(spider PROPERTIES
    CXX_EXTENSIONS OFF
)
