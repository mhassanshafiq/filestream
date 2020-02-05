cmake_minimum_required (VERSION 3.8)

project(common)

add_library(common STATIC
packet.cpp
transport_layer.cpp
file_handler.cpp
)