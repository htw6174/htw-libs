cmake_minimum_required(VERSION 3.13)

project(htw LANGUAGES C)

add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

if (HTW_STATIC)
    set(BUILD_SHARED_LIBS NO)
else (HTW_STATIC)
    set(BUILD_SHARED_LIBS YES)
endif (HTW_STATIC)

set(INCLUDE ${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src)

if (HTW_BUILD_TESTS)
    add_subdirectory(test)
endif (HTW_BUILD_TESTS)
