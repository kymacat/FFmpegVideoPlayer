cmake_minimum_required(VERSION 3.17)
project(VideoPlayer)
set(CMAKE_CXX_STANDARD 14)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
message(${CMAKE_BINARY_DIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

pkg_check_modules(SDL REQUIRED IMPORTED_TARGET
    SDL2
)

file(GLOB_RECURSE SOURCES
     "Sources/*.hpp"
     "Sources/*.cpp"
)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PkgConfig::LIBAV PkgConfig::SDL)
target_include_directories(${PROJECT_NAME} PRIVATE Sources/**)
