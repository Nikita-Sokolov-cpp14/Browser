cmake_minimum_required(VERSION 3.10)
project(browser)

add_subdirectory(database_manager)
add_subdirectory(spider)

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    database_manager
    spider
)
