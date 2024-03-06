#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/spi.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"

#include "rfm69_gpt.h"

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_MOSI 19
#define PIN_SCK 18

#define PIN_CS 17
#define PIN_RESET 21
#define PIN_INTR 20

static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 1);
    asm volatile("nop \n nop \n nop");
}

void interupt_handler(uint gpio, uint32_t events)
{   
    // Set the radio to standby mode
    uint8_t write_buffer[2] = {REG_OPMODE | RF69_WRITE_MASK, 0x04};
    cs_select();
    spi_write_blocking(SPI_PORT, write_buffer, 2);
    cs_deselect();
}

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

    rfm69_t rfm69 = {
        .spi = SPI_PORT,
        .cs_pin = PIN_CS,
        .reset_pin = PIN_RESET,
    };

    gpio_set_irq_enabled_with_callback(PIN_INTR, GPIO_IRQ_EDGE_RISE, true, &interupt_handler);


    // Initialize the RFM69 module
    rfm69_error_t error = rfm69_init(&rfm69);
    if (error != RFM69_OK)
    {
        printf("RFM69 initialization failed with error: %d\n", error);
        return 1;
    }

    rfm69_set_tx_power(&rfm69, 14, true);

    rfm69_print_registers(&rfm69);

    while (true)
    {   
        sleep_ms(1000);
        uint8_t data[] = "Hello, World!";
        rfm69_send(&rfm69, data, sizeof(data));
        printf("Sent Message\n");
    }
}
