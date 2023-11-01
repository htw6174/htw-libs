project(htw LANGUAGES C)

add_subdirectory(geomap)
# TODO: make inclusion optional, add build arg
#add_subdirectory(vulkan)
add_library(htw htw_core_math.c htw_random.c)

target_include_directories(htw PRIVATE ${INCLUDE})

set_target_properties(htw PROPERTIES PUBLIC_HEADER "${INCLUDE}/htw_core.h; ${INCLUDE}/htw_random.h; ${INCLUDE}/htw_geomap.h; ${INCLUDE}/htw_vulkan.h")
target_link_libraries(htw PRIVATE -lm)

install(TARGETS htw LIBRARY PUBLIC_HEADER DESTINATION lib)
