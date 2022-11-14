cmake_minimum_required(VERSION 3.16.0)
set(CMAKE_CXX_STANDARD 17)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
SET(EXTRA_COMPONENT_DIRS src )

#include_directories(${CMAKE_SOURCE_DIR}/include )

project(http-server-idf-test 
        VERSION 0.1.0 
        DESCRIPTION "Versuch eines Webservers"
        LANGUAGE CXX)

