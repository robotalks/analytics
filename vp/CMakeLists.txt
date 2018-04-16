cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenCV REQUIRED)
find_package(gflags REQUIRED CONFIG)
find_package(glog   REQUIRED CONFIG)
find_package(Libwebsockets REQUIRED)

include_directories(
    ${glog_INCLUDE_DIR}
    ${gflags_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
    "inc"
)

set(LIBS
    glog::glog
    gflags
    ${OpenCV_LIBS}
    mosquitto
    websockets
    cares
    ssl
    z
    mvnc
    pthread
)

add_library(vp
    src/vp/graph.cpp
    src/vp/graph_var.cpp
    src/vp/graph_builder.cpp
    src/vp/pool.cpp
    src/vp/dispatch.cpp
    src/vp/ingress.cpp
    src/vp/ingress/udp.cpp
    src/vp/op/image.cpp
    src/vp/cv/videocap.cpp
    src/vp/cv/operators.cpp
    src/vp/mqtt/mqtt.cpp
    src/movidius/ncs.cpp
)

add_executable(vpd src/vpd/app.cpp)
target_link_libraries(vpd vp ${LIBS})
