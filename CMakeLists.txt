cmake_minimum_required(VERSION 3.10)


include(pico_sdk_import.cmake)

# Set the project name and version
project(rfm69_pico VERSION 1.0 DESCRIPTION "RFM69 Library")

# Specify the C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
set(PICO_DEOPTIMIZED_DEBUG 1)

# # initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add the library
add_subdirectory(examples)
add_subdirectory(rfm69)


