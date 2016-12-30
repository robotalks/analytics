cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

include(common)

project(libcmn CXX)

add_library(cmn STATIC
    src/app.cpp
    src/pubsub.cpp
    src/mqtt.cpp
)
