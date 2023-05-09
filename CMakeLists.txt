cmake_minimum_required(VERSION 3.25)
project(FFmpegAPITutorial LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Debug)

include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/dep/FFmpeg/include)

set(ffmpeg_libs_dir ${PROJECT_SOURCE_DIR}/dep/FFmpeg/lib)
link_directories(${ffmpeg_libs_dir})
file(GLOB ffmpeg_dylibs ${ffmpeg_libs_dir}/*.dylib)

set(src_dir ${PROJECT_SOURCE_DIR}/src)
file(GLOB src_files ${src_dir}/*.cpp)

set(demo_dir ${PROJECT_SOURCE_DIR}/demo)
file(GLOB demo_files ${demo_dir}/*.cpp)

foreach (demo ${demo_files})
    string(REGEX MATCH "[^/]+$" demo_file ${demo})
    string(REPLACE ".cpp" "" demo_name ${demo_file})
    add_executable(${demo_name} ${demo} ${src_files})
    target_link_libraries(${demo_name} ${ffmpeg_dylibs})
endforeach ()
