cmake_minimum_required(VERSION 3.5)
project(qoa)

add_library(qoa include/qoa.h qoa.c)
target_include_directories(qoa PUBLIC include/)