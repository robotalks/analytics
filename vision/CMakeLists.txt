cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

include(common)

project(vision CXX)

find_library(COMMON_LIB cmn)
find_library(MOSQUITTO_LIB mosquitto)
find_library(CARES_LIB cares)
find_library(CPPNETLIB_URI_LIB cppnetlib-uri)
find_library(BOOST_SYSTEM_LIB boost_system)
find_library(BOOST_THREAD_LIB boost_thread)
find_package(OpenCV REQUIRED)
find_library(AVFORMAT_LIB avformat)
find_library(AVCODEC_LIB avcodec)
find_library(SWSCALE_LIB swscale)
find_library(SWRESAMPLE_LIB swresample)
find_library(AVUTIL_LIB avutil)
find_library(OPENSSL_LIB ssl)
find_library(CRYPTO_LIB crypto)
find_library(LIBZ_LIB z)
find_library(BZIP2_LIB bz2)

set(VISION_LIBS
    ${COMMON_LIB}
    ${MOSQUITTO_LIB}
    ${CARES_LIB}
    ${CPPNETLIB_URI_LIB}
    ${BOOST_THREAD_LIB}
    ${BOOST_SYSTEM_LIB}
    ${OpenCV_LIBS}
    ${AVFORMAT_LIB}
    ${AVCODEC_LIB}
    ${SWSCALE_LIB}
    ${SWRESAMPLE_LIB}
    ${AVUTIL_LIB}
    ${OPENSSL_LIB}
    ${CRYPTO_LIB}
    ${LIBZ_LIB}
    ${BZIP2_LIB}
)

set(VISION_SRCS
    src/model.cpp
    src/pipeline.cpp
    src/analyzer.cpp
    src/face_detector.cpp
    src/detection.cpp
)

add_executable(vision ${VISION_SRCS} src/vision.cpp)
target_link_libraries(vision -Wl,--start-group ${VISION_LIBS} -Wl,--end-group -lpthread)

add_executable(vision-perf ${VISION_SRCS} src/perf.cpp)
target_link_libraries(vision-perf -Wl,--start-group ${VISION_LIBS} -Wl,--end-group)
