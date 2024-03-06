cmake_minimum_required(VERSION 3.10)


include(pico_sdk_import.cmake)

# Set the project name and version
project(RFM69 VERSION 1.0 DESCRIPTION "RFM69 Library")

# Specify the C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
set(PICO_DEOPTIMIZED_DEBUG 1)

# # initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(rfm69
    src/main.c
    src/rfm69.c
)

target_include_directories(rfm69 PRIVATE include)

target_link_libraries(rfm69 pico_stdlib hardware_spi)

#enable USB output
pico_enable_stdio_usb(rfm69 0)
#enable UART output
pico_enable_stdio_uart(rfm69 1)


