add_library(rfm69)

target_sources(rfm69 PRIVATE src/rfm69.c)
target_include_directories(rfm69 PUBLIC include)

# Link to pico_stdlib if you use standard library functions
target_link_libraries(rfm69 pico_stdlib hardware_spi)
