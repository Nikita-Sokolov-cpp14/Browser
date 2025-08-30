cmake_minimum_required(VERSION 3.10)
project(BoostExample)

include_directories(C:/dev/boost/include/boost-1_89)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE)