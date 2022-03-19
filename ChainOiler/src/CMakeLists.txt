# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

# set(component_srcs "../lib/led_strip_rmt_ws2812.c")

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS "${app_sources}"
                       PRIV_REQUIRES "driver" )

                       # idf_component_register(SRCS "${component_srcs} ${app_sources}"
#                        INCLUDE_DIRS "include"
#                        PRIV_INCLUDE_DIRS ""
#                        PRIV_REQUIRES "driver"
#                        REQUIRES "")

#idf_component_register(SRCS ${app_sources})
