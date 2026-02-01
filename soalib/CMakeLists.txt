cmake_minimum_required(VERSION 4.2)

project(soalib LANGUAGES C CXX)

file(GLOB_RECURSE _SOURCES *.c *.cpp *.cxx)
file(GLOB_RECURSE _HEADERS *.h *.hpp *.hxx)

set(_INCLUDE 
    ${CMAKE_SOURCE_DIR}
)

set(_LIBRARY
    ""
)

add_library(${PROJECT_NAME} STATIC ${_SOURCES} ${_HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ${_INCLUDE})
target_link_libraries(${PROJECT_NAME} PUBLIC ${_LIBRARY})