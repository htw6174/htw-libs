project(htw LANGUAGES C)

add_library(htw_geomap htw_geomap_chunkmap.c htw_geomap_hexgrid.c htw_geomap_tilemap.c htw_geomap_valuemap.c htw_geomap_generators.c htw_geomap_spatialStorage.c)
target_include_directories(htw_geomap PRIVATE ${INCLUDE})
