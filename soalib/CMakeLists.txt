cmake_minimum_required(VERSION 4.2)

project(soalib LANGUAGES C CXX)

file(GLOB_RECURSE _SOURCES *.c *.cpp *.cxx)
file(GLOB_RECURSE _HEADERS *.h *.hpp *.hxx)

add_library(${PROJECT_NAME} STATIC ${_SOURCES} ${_HEADERS})