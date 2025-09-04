cmake_minimum_required(VERSION 3.10)
project(BoostExample)

find_package(LibXml2 REQUIRED)

add_executable(my_app main.cpp
                        spider/spider.cpp
                        spider/indexer/indexer.cpp
                        spider/parser/parser.cpp)

target_include_directories(my_app PRIVATE ${LIBXML2_INCLUDE_DIRS})

target_link_libraries(my_app
    PRIVATE
        ${LIBXML2_LIBRARIES}
)
