cmake_minimum_required(VERSION 3.18)
project(visibility_ex)

add_compile_definitions(LIBEXPORT)

add_library(test
                SHARED
            test_lib.cpp)

set_property(TARGET test PROPERTY VISIBILITY_INLINES_HIDDEN ON)
set_property(TARGET test PROPERTY POSITION_INDEPENDENT_CODE ON)

add_executable(main main.cpp)
target_link_libraries(main test)    