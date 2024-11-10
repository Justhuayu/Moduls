cmake_minimum_required(VERSION 3.10)
project(PushStream)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#忽略deprecated警告
add_compile_options(-Wno-deprecated-declarations)

#添加项目根目录路径
set(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR})
add_definitions(-DPROJECT_ROOT_DIR="${PROJECT_ROOT_DIR}")

#添加include文件夹，存放h文件
include_directories(${CMAKE_SOURCE_DIR}/include)

#添加 uuid 库
find_package(Threads REQUIRED)
find_library(UUID_LIB uuid REQUIRED)

#配置本地opencv
set(OpenCV_DIR "${CMAKE_SOURCE_DIR}/../_libs/opencv4.10.0/lib/cmake/opencv4")
find_package(OpenCV REQUIRED)
message(STATUS "Using OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV Include Directory: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV Libraries: ${OpenCV_LIBS}")

#配置本地ffmpeg
set(FFMPEG_DIR "${CMAKE_SOURCE_DIR}/../_libs/ffmpeg")
set(ENV{PKG_CONFIG_PATH} "${FFMPEG_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWSCALE REQUIRED libswscale)
message(STATUS "FFmpeg libavcodec Include Directory: ${AVCODEC_INCLUDE_DIRS}")
message(STATUS "FFmpeg libavformat Include Directory: ${AVFORMAT_INCLUDE_DIRS}")
message(STATUS "FFmpeg libavutil Include Directory: ${AVUTIL_INCLUDE_DIRS}")
message(STATUS "FFmpeg libswscale Include Directory: ${SWSCALE_INCLUDE_DIRS}")
message(STATUS "FFmpeg libavcodec Libraries: ${AVCODEC_LIBRARIES}")
message(STATUS "FFmpeg libavformat Libraries: ${AVFORMAT_LIBRARIES}")
message(STATUS "FFmpeg libavutil Libraries: ${AVUTIL_LIBRARIES}")
message(STATUS "FFmpeg libswscale Libraries: ${SWSCALE_LIBRARIES}")

#添加src路径下所有cpp文件
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(PushStream main.cpp ${SOURCES})

target_link_libraries(PushStream PRIVATE
        ${OpenCV_LIBS}
        ${AVCODEC_LIBRARIES}
        ${AVFORMAT_LIBRARIES}
        ${AVUTIL_LIBRARIES}
        ${SWSCALE_LIBRARIES}
        ${UUID_LIB}
        Threads::Threads
)
