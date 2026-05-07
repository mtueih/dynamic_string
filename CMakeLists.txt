cmake_minimum_required(VERSION 4.2)
project(dynamic_string LANGUAGES C)

find_package(attrs REQUIRED)
find_package(safe_calc REQUIRED)

add_library(dstr_adt STATIC src/dynamic_string.c)
target_include_directories(dstr_adt PUBLIC include)
target_link_libraries(dstr_adt PUBLIC attrs::attrs)
target_link_libraries(dstr_adt PRIVATE safe_calc::safe_calc)

add_executable(dynamic_string_test src/main.c)
target_link_libraries(dynamic_string_test PRIVATE dstr_adt)
