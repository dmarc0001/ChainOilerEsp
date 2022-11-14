# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

list( APPEND COMPONENTS nvs_flash )

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources} 
                       INCLUDE_DIRS "../include"
                       PRIV_REQUIRES nvs_flash
                       )

