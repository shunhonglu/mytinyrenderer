cmake_minimum_required(VERSION 3.0.0)
project(mytinyrender)

add_compile_options(-Wall -Wextra -Weffc++ -pedantic -std=c++11 -O3)

add_executable(mytinyrender main.cpp geometry.cpp mipmap.cpp model.cpp our_gl.cpp tgaimage.cpp)
