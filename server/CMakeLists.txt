cmake_minimum_required(VERSION 3.10)
project(aarc-rc-converter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

include_directories(${CMAKE_SOURCE_DIR}/../nlohmann)
include_directories(${CMAKE_SOURCE_DIR}/../src)
include_directories(${CMAKE_SOURCE_DIR}/asio-1.30.2/include)
include_directories(${CMAKE_SOURCE_DIR}/Crow/include)
include_directories(${CMAKE_SOURCE_DIR}/jwt-cpp/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(aarc-rc-backend
    ../src/converter.cc
    ../src/geometry.cc
    ../src/rc.cc
    src/main.cc
    src/utils.cc
    src/winit.cc
)
