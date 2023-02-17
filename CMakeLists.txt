cmake_minimum_required(VERSION 3.0.0)
project(mytinyrender)

set(CMAKE_BUILD_TYPE Debug)

add_compile_options(-Wall -Wextra -Weffc++ -pedantic -std=c++11)

add_executable(mytinyrender main.cpp geometry.cpp mipmap.cpp model.cpp our_gl.cpp tgaimage.cpp)
