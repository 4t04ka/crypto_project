cmake_minimum_required(VERSION 3.27)
project(CRYPTO_PROJECT)

set(CMAKE_CXX_STANDARD 14)

add_executable(go main.cpp)
#
add_subdirectory(td)
target_link_libraries(go PUBLIC Td::TdStatic)

add_subdirectory(BING_API)
target_link_libraries(go PRIVATE bing_lib)
#find_package(td/Td REQUIRED)
#add_executable(my_program main.cpp)
#target_link_libraries(my_program Td::TdClient Td::TdCore Td::TdUtils)