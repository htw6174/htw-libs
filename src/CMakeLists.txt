project(htw LANGUAGES C)

# TODO: make inclusion optional, add build arg
add_library(htw htw_core_math.c htw_random.c)
add_subdirectory(geomap)
if (HTW_VULKAN)
    add_subdirectory(vulkan)
endif (HTW_VULKAN)

target_include_directories(htw PUBLIC ${INCLUDE})

set_target_properties(htw PROPERTIES PUBLIC_HEADER "${INCLUDE}/htw_core.h; ${INCLUDE}/htw_random.h; ${INCLUDE}/htw_geomap.h; ${INCLUDE}/htw_vulkan.h")
target_link_libraries(htw PRIVATE -lm)

install(TARGETS htw LIBRARY PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
