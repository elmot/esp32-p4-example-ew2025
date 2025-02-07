cmake_minimum_required(VERSION 3.16)

# -DSDKCONFIG_DEFAULTS="sdkconfig.defaults.esp32_p4_function_ev_board;sdkconfig.defaults" -DBUILD_BOARD="esp32_p4_function_ev_board_noglib" -DIDF_TARGET=esp32p4

set(BUILD_BOARD esp32_p4_function_ev_board_noglib)
set(IDF_TARGET esp32p4)

set(ENV{BUILD_BOARD} ${BUILD_BOARD})

set(COMPILER_OPTIMIZATION_NONE 1)

# "Trim" the build. Include the minimal set of components; main and anything it depends on.
set(COMPONENTS  main esp32_p4_function_ev_board_noglib georgik__sdl)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp32-p4-example-ew2025 C CXX)

get_filename_component(configName "${CMAKE_BINARY_DIR}" NAME)
littlefs_create_partition_image(assets assets FLASH_IN_PROJECT)
add_link_options(--enable-non-contiguous-regions)
