cmake_minimum_required(VERSION 3.10)
project(dynamic_string LANGUAGES C)

add_subdirectory(deps/portable_attributes)

add_library(dstr_adt STATIC src/dynamic_string.c)
target_include_directories(dstr_adt PUBLIC include)
target_link_libraries(dstr_adt PRIVATE portable_attributes::portable_attributes)

add_executable(dynamic_string_test src/main.c)
target_link_libraries(dynamic_string_test PRIVATE dstr_adt)
