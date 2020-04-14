cmake_minimum_required(VERSION 3.13)
project(smash)

set(CMAKE_CXX_STANDARD 14)

add_executable(smash smash.cpp signals.cpp signals.h Commands.cpp Commands.h)