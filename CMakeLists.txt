cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-O3")

project(linp VERSION 1.0 LANGUAGES CXX)

# main engine
set(SOURCE
  "src/linp.cpp"
)

add_executable(linp ${SOURCE})

target_include_directories(linp PRIVATE "include")