#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/spi.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"


#include "rfm69.h"

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_MOSI 19
#define PIN_SCK 18

#define PIN_CS 17
#define PIN_RESET 20
#define PIN_INTR 21


int main()
{
    stdio_init_all();

    // initalize SPI interface
    spi_init(SPI_PORT, 1000 * 1000);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    // Make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(PIN_MISO, PIN_MOSI, PIN_SCK, GPIO_FUNC_SPI));

    // Create the struct for the RFM69 module
    rfm69_t rfm69 = {
        .spi = SPI_PORT,
        .cs_pin = PIN_CS,
        .reset_pin = PIN_RESET,
        .interrupt_pin = PIN_INTR,
    };

    // Initialize the RFM69 module
    rfm69_error_t error = rfm69_init(&rfm69);
    if (error != RFM69_OK)
    {
        printf("RFM69 initialization failed with error: %d\n", error);
        while (true) tight_loop_contents();
    }

    rfm69_print_registers(&rfm69);

    rfm69_set_mode_rx(&rfm69);

    while (true)
    {
        sleep_ms(1000);

        if (rfm69_is_message_available(&rfm69)){
            message_t message;
            rfm69_receive(&rfm69, &message);

            // Print the received message bytes
            printf("Received message: ");
            for (int i = 0; i < message.len; i++)
            {
                printf("%02X ", message.data[i]);
            }
            printf("\n");
            
        }

    }
}
