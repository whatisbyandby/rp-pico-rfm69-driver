add_executable(basic_transmitter main.c)

# Include directories of your library
target_include_directories(basic_transmitter PRIVATE ${CMAKE_SOURCE_DIR}/rfm69/include)

# Link the executable with the library
target_link_libraries(basic_transmitter rfm69)

# Link to pico_stdlib if you use standard library functions and the pico specific libraries
target_link_libraries(basic_transmitter pico_stdlib hardware_spi)

# enable usb output, disable uart output
pico_enable_stdio_usb(basic_transmitter 1)
pico_enable_stdio_uart(basic_transmitter 1)

pico_add_extra_outputs(basic_transmitter)
