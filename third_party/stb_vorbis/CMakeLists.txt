cmake_minimum_required(VERSION 3.5)
project(stb_vorbis)

add_library(stb_vorbis include/stb_vorbis.h stb_vorbis.c)
target_include_directories(stb_vorbis PUBLIC include/)