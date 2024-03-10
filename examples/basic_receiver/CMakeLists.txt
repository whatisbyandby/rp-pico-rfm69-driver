add_executable(basic_receiver main.c)

# Include directories of your library
target_include_directories(basic_receiver PRIVATE ${CMAKE_SOURCE_DIR}/rfm69/include)

# Link the executable with the library
target_link_libraries(basic_receiver rfm69)

# Link to pico_stdlib if you use standard library functions and the pico specific libraries
target_link_libraries(basic_receiver pico_stdlib hardware_spi)

pico_add_extra_outputs(basic_receiver)
