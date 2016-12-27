cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

include(common)

project(vision CXX)

find_library(COMMON_LIB cmn)
find_package(OpenCV REQUIRED)
find_library(AVDEVICE_LIB avdevice)
find_library(AVFILTER_LIB avfilter)
find_library(AVFORMAT_LIB avformat)
find_library(AVCODEC_LIB avcodec)
find_library(SWSCALE_LIB swscale)
find_library(SWRESAMPLE_LIB swresample)
find_library(AVUTIL_LIB avutil)

set(VISION_LIBS
    ${COMMON_LIB}
    ${OpenCV_LIBS}
    ${AVDEVICE_LIB}
    ${AVFILTER_LIB}
    ${AVFORMAT_LIB}
    ${AVCODEC_LIB}
    ${SWSCALE_LIB}
    ${SWRESAMPLE_LIB}
    ${AVUTIL_LIB}
)

set(VISION_SRCS
    src/model.cpp
    src/analyzer.cpp
    src/face_detector.cpp
    src/detection.cpp
)

add_executable(vision ${VISION_SRCS} src/vision.cpp)
target_link_libraries(vision -Wl,--start-group ${VISION_LIBS} -Wl,--end-group)

add_executable(vision-perf ${VISION_SRCS} src/perf.cpp)
target_link_libraries(vision-perf -Wl,--start-group ${VISION_LIBS} -Wl,--end-group)
