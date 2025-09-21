cmake_minimum_required(VERSION 3.10)
project(BoostExample)

find_package(LibXml2 REQUIRED)
find_package(Boost REQUIRED COMPONENTS locale)
find_package(PostgreSQL REQUIRED)

include_directories(${PostgreSQL_INCLUDE_DIR})

add_executable(my_app main.cpp
                        spider/spider.cpp
                        spider/indexer/indexer.cpp
                        spider/parser/parser.cpp
                        spider/database_manager/database_manager.cpp)

target_include_directories(my_app
    PRIVATE
        ${LIBXML2_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(my_app
    PRIVATE
        ${LIBXML2_LIBRARIES}
        ${Boost_LIBRARIES}
        ${PostgreSQL_LIBRARIES}
        pqxx
)
