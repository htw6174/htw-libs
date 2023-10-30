project(htw LANGUAGES C)

#add_compile_definitions($<$<CONFIG:Debug>:VK_DEBUG>) # use to enable Vulkan validation layers

add_library(htw_vulkan SHARED htw_vulkan.c)
target_include_directories(htw_vulkan PRIVATE ${INCLUDE})

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS})
target_link_libraries(htw_vulkan PUBLIC ${SDL2_LIBRARIES} ${Vulkan_LIBRARIES})
