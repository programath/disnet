cmake_minimum_required(VERSION 3.7)
project(disnet)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES convnet.cpp util.h test.cpp)
add_executable(disnet ${SOURCE_FILES})