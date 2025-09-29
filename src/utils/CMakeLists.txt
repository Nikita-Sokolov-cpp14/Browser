cmake_minimum_required(VERSION 3.0.0)

find_package(LibXml2 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS locale)

add_library(utils
    secondary_function.cpp
)

target_include_directories(utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    ${CMAKE_SOURCE_DIR}/..
)

target_link_libraries(utils PUBLIC
    LibXml2::LibXml2
)

target_compile_features(utils PRIVATE cxx_std_17)

set_target_properties(utils PROPERTIES
    CXX_EXTENSIONS OFF
)
