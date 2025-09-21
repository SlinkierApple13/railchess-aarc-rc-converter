cmake_minimum_required(VERSION 3.10)
project(aarc-rc-converter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/nlohmann)

add_executable(aarc-rc-converter
    src/converter.cc
    src/geometry.cc    
    src/main.cc
    src/rc.cc
)
