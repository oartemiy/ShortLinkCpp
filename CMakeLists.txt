cmake_minimum_required(VERSION 3.10.0)
project(BackendCpp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-pthread)

find_package(libpqxx CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 20)

add_executable(BackendCpp
src/main.cpp
src/gen_code/gen_code.cpp
src/storage/LinkManager.cpp
src/handlers/handlers.cpp)

target_link_libraries(BackendCpp PRIVATE libpqxx::pqxx)



