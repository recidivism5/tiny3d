cmake_minimum_required(VERSION 3.5)
project(qoi)

add_library(qoi include/qoi.h qoi.c)
target_include_directories(qoi PUBLIC include/)