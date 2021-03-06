cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

include(common)

project(vision CXX)

find_library(COMMON_LIB cmn)
find_library(MOSQUITTO_LIB mosquitto)
find_library(CPPNETLIB_URI_LIB cppnetlib-uri)
find_library(BOOST_SYSTEM_LIB boost_system)
find_library(BOOST_THREAD_LIB boost_thread)
find_package(OpenCV REQUIRED)
# find_library(AVFORMAT_LIB avformat)
# find_library(AVCODEC_LIB avcodec)
# find_library(SWSCALE_LIB swscale)
# find_library(SWRESAMPLE_LIB swresample)
# find_library(AVUTIL_LIB avutil)
find_library(LIBZ_LIB z)
find_library(BZIP2_LIB bz2)

set(VISION_LIBS
    ${COMMON_LIB}
    ${MOSQUITTO_LIB}
    ${CPPNETLIB_URI_LIB}
    ${BOOST_THREAD_LIB}
    ${BOOST_SYSTEM_LIB}
    ${OpenCV_LIBS}
    # ${AVFORMAT_LIB}
    # ${AVCODEC_LIB}
    # ${SWSCALE_LIB}
    # ${SWRESAMPLE_LIB}
    # ${AVUTIL_LIB}
    ${LIBZ_LIB}
    ${BZIP2_LIB}
)

add_executable(cap src/cap.cpp)
target_link_libraries(cap -Wl,--start-group ${VISION_LIBS} -Wl,--end-group -lpthread)

add_executable(vision
    src/vision/model.cpp
    src/vision/pipeline.cpp
    src/vision/app.cpp
    src/vision/capture_cv.cpp
    src/vision/capture_udp.cpp
    src/detect/face.cpp
    src/detect/motion_simple.cpp
    src/detect/track.cpp
    src/vision.cpp
)
target_link_libraries(vision -Wl,--start-group ${VISION_LIBS} -Wl,--end-group -lpthread)

add_executable(vision-perf
    src/vision/model.cpp
    src/vision/pipeline.cpp
    src/vision/app.cpp
    src/detect/face.cpp
    src/perf/analyzer.cpp
    src/perf/detection.cpp
    src/perf/perf.cpp
)
target_link_libraries(vision-perf -Wl,--start-group ${VISION_LIBS} -Wl,--end-group)
