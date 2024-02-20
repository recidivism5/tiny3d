cmake_minimum_required(VERSION 3.5)
project(stb_image)

add_library(stb_image include/stb_image.h stb_image.c)
target_include_directories(stb_image PUBLIC include/)