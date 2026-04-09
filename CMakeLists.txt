cmake_minimum_required(VERSION 3.10.0)
project(BackendCpp VERSION 0.1.0 LANGUAGES C CXX)

add_compile_options(-pthread)

add_link_options(-sqlite3)

set(CMAKE_CXX_STANDARD 20)

add_executable(BackendCpp
src/main.cpp
src/gen_code/gen_code.cpp
src/storage/LinkManager.cpp
src/handlers/handlers.cpp)


# target_precompile_headers(BackendCpp PUBLIC httplib.h json.hpp)

