project(htw_libs_test LANGUAGES C)

add_executable(htw_libs_test main.c)

target_include_directories(htw_libs_test PRIVATE ${INCLUDE})
target_link_libraries(htw_libs_test PRIVATE -lm htw)

install(TARGETS htw_libs_test RUNTIME DESTINATION bin)
