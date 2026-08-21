cmake_minimum_required(VERSION 3.20)

project(vnpu_hal LANGUAGES CXX)

add_library(vnpu_hal STATIC
    src/vnpu-hal.cpp
)

target_compile_features(vnpu_hal PUBLIC cxx_std_20)
target_include_directories(vnpu_hal
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../linux-driver/include
)

target_compile_options(vnpu_hal PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(vnpuctl
    src/vnpuctl.cpp
)

target_link_libraries(vnpuctl PRIVATE vnpu_hal)
target_compile_features(vnpuctl PRIVATE cxx_std_20)
target_compile_options(vnpuctl PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
set_target_properties(vnpuctl PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../tools
)
