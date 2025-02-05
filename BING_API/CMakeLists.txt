cmake_minimum_required(VERSION 3.27)

set(CMAKE_CXX_STANDARD 14)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)


# Линковка с curl и OpenSSL


add_library(bing_lib STATIC
    src/bing_api.cpp
)
target_include_directories(bing_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)



target_link_libraries(bing_lib PUBLIC CURL::libcurl OpenSSL::SSL OpenSSL::Crypto fmt::fmt nlohmann_json::nlohmann_json)

