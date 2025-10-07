cmake_minimum_required(VERSION 3.0.0)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS locale)

add_library(page_loader
    page_loader.cpp
)

target_include_directories(page_loader PUBLIC
    ${PROJECT_SOURCE_DIR}/
)

target_link_libraries(page_loader PRIVATE
    utils
    OpenSSL::SSL
    Boost::locale
)
