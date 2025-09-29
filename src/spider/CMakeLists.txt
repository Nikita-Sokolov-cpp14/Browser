cmake_minimum_required(VERSION 3.0.0)

find_package(LibXml2 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS locale)

add_library(spider
    spider.cpp
    parser/parser.cpp
    indexer/indexer.cpp
)

target_include_directories(spider PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(spider PUBLIC
    database_manager
    LibXml2::LibXml2
    Boost::locale
    OpenSSL::SSL
    utils
)

target_compile_features(spider PRIVATE cxx_std_17)

set_target_properties(spider PROPERTIES
    CXX_EXTENSIONS OFF
)
