cmake_minimum_required(VERSION 3.0.0)

find_package(LibXml2 REQUIRED)
find_package(Boost REQUIRED COMPONENTS locale)

add_library(parser
    parser.cpp
)

target_include_directories(parser PUBLIC
    ${PROJECT_SOURCE_DIR}/
)

target_link_libraries(parser PUBLIC
    LibXml2::LibXml2
    Boost::locale
)
