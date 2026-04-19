cmake_minimum_required(VERSION 3.10.0)
project(BackendCpp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Drogon CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fcoroutines")

add_executable(BackendCpp
src/main.cpp
src/gen_code/gen_code.cpp
src/storage/LinkManager.cpp
src/handlers/handlers.cpp)

target_link_libraries(BackendCpp PRIVATE Drogon::Drogon nlohmann_json::nlohmann_json)
