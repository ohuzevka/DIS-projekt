# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
add_compile_options("-Wno-format")
project(bsp-kaluga-display-audio-example)
spiffs_create_partition_image(storage spiffs FLASH_IN_PROJECT)
