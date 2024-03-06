#ifndef RFM69_H
#define RFM69_H

#include "pico/stdlib.h"
#include "rfm69_registers.h"
#include "hardware/spi.h"

// RF69 Write Mask
#define RF69_WRITE_MASK 0x80

typedef enum
{
    RFM69_OK = 0,
    RFM69_SPI_READ_ERROR,
    RFM69_SPI_WRITE_ERROR,
} rfm69_error_t;

typedef struct rfm69_t
{
    spi_inst_t *spi;
    uint cs_pin;
    uint reset_pin;
} rfm69_t;

uint8_t rfm69_get_revision(rfm69_t *rfm69, uint8_t *revision);

// Reset the RFM69 module
void rfm69_reset(rfm69_t *rfm69);

// Set the operation mode
rfm69_error_t rfm69_set_mode(rfm69_t *rfm69, uint8_t mode);

rfm69_error_t rfm69_init(rfm69_t *rfm69);

rfm69_error_t rfm69_send(rfm69_t *rfm69, uint8_t *data, uint8_t len);

rfm69_error_t rfm69_set_tx_power(rfm69_t *rfm69, uint8_t tx_power, bool is_high_power_module);

rfm69_error_t rfm69_print_registers(rfm69_t *rfm69);

#endif // RFM69_H
