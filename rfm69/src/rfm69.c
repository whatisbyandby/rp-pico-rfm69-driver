#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include "rfm69.h"
#include "string.h"

static rfm69_t *rfm69_inst;

static inline void cs_select(rfm69_t *rfm69)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(rfm69->cs_pin, 0); // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(rfm69_t *rfm69)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(rfm69->cs_pin, 1);
    asm volatile("nop \n nop \n nop");
}

rfm69_error_t write_register(rfm69_t *rfm69, uint8_t addr, uint8_t data)
{
    uint8_t write_buffer[2] = {addr | RF69_WRITE_MASK, data};
    cs_select(rfm69);
    int num_bytes_written = spi_write_blocking(rfm69->spi, write_buffer, 2);
    cs_deselect(rfm69);
    return num_bytes_written == 2 ? RFM69_OK : RFM69_SPI_WRITE_ERROR;
}

rfm69_error_t read_register(rfm69_t *rfm69, uint8_t addr, uint8_t *data)
{
    cs_select(rfm69);
    bool success = spi_write_blocking(rfm69->spi, &addr, 1) == 1;
    success = success | spi_read_blocking(rfm69->spi, 0, data, 1) == 1;
    cs_deselect(rfm69);
    return success ? RFM69_OK : RFM69_SPI_READ_ERROR;
}

rfm69_error_t write_packet(rfm69_t *rfm69, uint8_t *data, uint8_t len)
{
    cs_select(rfm69);
    uint8_t write_buffer[1] = {REG_FIFO | RF69_WRITE_MASK};
    int num_bytes_written = spi_write_blocking(rfm69->spi, write_buffer, 1);
    uint8_t length_buffer[1] = {0};
    spi_write_read_blocking(rfm69->spi, &len, length_buffer, 1);
    uint8_t data_buffer[len];
    spi_write_read_blocking(rfm69->spi, data, data_buffer, len);
    cs_deselect(rfm69);
    return RFM69_OK;
}

rfm69_error_t burst_write(rfm69_t *rfm69, uint8_t addr, uint8_t *data, uint8_t len)
{
    uint8_t write_addr = addr | RF69_WRITE_MASK;
    cs_select(rfm69);
    bool success = spi_write_blocking(rfm69->spi, &write_addr, 1) == 1;
    success = success | spi_write_blocking(rfm69->spi, data, len) == len;
    cs_deselect(rfm69);
    return success ? RFM69_OK : RFM69_SPI_WRITE_ERROR;
}

rfm69_error_t burst_read(rfm69_t *rfm69, uint8_t addr, uint8_t *data, uint8_t len)
{
    cs_select(rfm69);
    bool success = spi_write_blocking(rfm69->spi, &addr, 1) == 1;
    success = success | spi_read_blocking(rfm69->spi, 0, data, len) == len;
    cs_deselect(rfm69);
    return success ? RFM69_OK : RFM69_SPI_READ_ERROR;
}

void rfm69_reset(rfm69_t *rfm69)
{
    // Pull the reset pin high to reset the RFM69
    gpio_put(rfm69->reset_pin, 1);
    sleep_ms(10);
    // Pull the reset pin low to return to normal operation
    gpio_put(rfm69->reset_pin, 0);
    sleep_ms(10);
}

rfm69_error_t rfm69_get_revision(rfm69_t *rfm69, uint8_t *revision)
{
    return read_register(rfm69, REG_VERSION, revision);
}

rfm69_error_t rfm69_set_mode(rfm69_t *rfm69, uint8_t mode)
{
    uint8_t current_mode;

    // Read the current mode
    rfm69_error_t error = read_register(rfm69, REG_OPMODE, &current_mode);
    current_mode &= ~0x1C;
    current_mode |= (mode & 0x1C);

    // Set the new mode
    write_register(rfm69, REG_OPMODE, current_mode);
    uint8_t status = 0;
    read_register(rfm69, REG_IRQFLAGS1, &status);
    while (!status & 0x80)
        ;
}

rfm69_error_t rfm69_set_default_fifo_threshold(rfm69_t *rfm69)
{
    return write_register(rfm69, REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | 0x0F);
}

rfm69_error_t set_sync_word(rfm69_t *rfm69, uint8_t *sync_words, uint8_t len)
{
    if (len > 8)
    {
        return RFM69_SPI_WRITE_ERROR;
    }
    write_register(rfm69, REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_2);
    return burst_write(rfm69, REG_SYNCVALUE1, sync_words, len);
}

rfm69_error_t set_modem_registers(rfm69_t *rfm69, ModemConfig *config)
{

    rfm69_error_t err = burst_write(rfm69, REG_DATAMODUL, &config->reg_02, 5);
    sleep_ms(10);
    if (err != RFM69_OK)
        return err;

    err = burst_write(rfm69, REG_RXBW, &config->reg_19, 2);
    sleep_ms(10);
    err = write_register(rfm69, REG_PACKETCONFIG1, config->reg_37);

    if (err != RFM69_OK)
        return err;
    return RFM69_OK;
}

rfm69_error_t set_modem_config(rfm69_t *rfm69, ModemConfigChoice index)
{
    ModemConfig cfg;
    memcpy(&cfg, &MODEM_CONFIG_TABLE[index], sizeof(ModemConfig));
    return set_modem_registers(rfm69, &cfg);
}

rfm69_error_t rfm69_set_preamble_length(rfm69_t *rfm69, uint16_t length)
{
    rfm69_error_t err = write_register(rfm69, REG_PREAMBLEMSB, (uint8_t)(length >> 8));
    if (err != RFM69_OK)
        return err;
    err = write_register(rfm69, REG_PREAMBLELSB, (uint8_t)(length & 0xFF));
    return err;
}

rfm69_error_t rfm69_set_frequency(rfm69_t *rfm69, uint32_t frequency)
{
    uint32_t frf = (frequency * 1000000) / RF69_FSTEP;
    rfm69_error_t err = write_register(rfm69, REG_FRFMSB, (frf >> 16) & 0xFF);
    if (err != RFM69_OK)
        return err;
    err = write_register(rfm69, REG_FRFMID, (frf >> 8) & 0xFF);
    if (err != RFM69_OK)
        return err;
    err = write_register(rfm69, REG_FRFLSB, frf & 0xFF);
    return err;
}

rfm69_error_t rfm69_set_tx_power(rfm69_t *rfm69, uint8_t tx_power, bool is_high_power_module)
{
    uint8_t palevel;

    if (is_high_power_module)
    {
        if (tx_power < -2)
            tx_power = -2; // RFM69HW only works down to -2.
        if (tx_power <= 13)
        {
            // -2dBm to +13dBm
            // Need PA1 exclusivelly on RFM69HW
            palevel = RF_PALEVEL_PA1_ON | ((tx_power + 18) &
                                           RF_PALEVEL_OUTPUTPOWER_11111);
        }
        else if (tx_power >= 18)
        {
            // +18dBm to +20dBm
            // Need PA1+PA2
            // Also need PA boost settings change when tx is turned on and off, see setModeTx()
            palevel = RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON | ((tx_power + 11) & RF_PALEVEL_OUTPUTPOWER_11111);
        }
        else
        {
            // +14dBm to +17dBm
            // Need PA1+PA2
            palevel = RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON | ((tx_power + 14) & RF_PALEVEL_OUTPUTPOWER_11111);
        }
    }
    else
    {
        if (tx_power < -18)
            tx_power = -18;
        if (tx_power > 13)
            tx_power = 13; // limit for RFM69W
        palevel = RF_PALEVEL_PA0_ON | ((tx_power + 18) & RF_PALEVEL_OUTPUTPOWER_11111);
    }
    rfm69->_power = tx_power;
    write_register(rfm69, REG_PALEVEL, palevel);
}

rfm69_error_t rfm69_read_fifo(rfm69_t *rfm69){
    uint8_t length;
    read_register(rfm69, REG_FIFO, &length);
    
    //read the 4 byte header
    uint8_t header[4];
    burst_read(rfm69, REG_FIFO, header, 4);

    burst_read(rfm69, REG_FIFO, rfm69->_buffer, length - 4);
    rfm69->_bufLen = length - 4;

    rfm69->message_available = true;
}




rfm69_error_t rfm69_handle_interrupt(rfm69_t *rfm69) {

    uint8_t interrupt_reason;
    read_register(rfm69, REG_IRQFLAGS2, &interrupt_reason);

    // if mode is TX and packet sent, return to standby mode
    if (rfm69->_mode == ModeTx && (interrupt_reason & RF_IRQFLAGS2_PACKETSENT)) {
        rfm69_set_mode(rfm69, RF_OPMODE_STANDBY);
        rfm69->_mode = ModeIdle;
    }
    // if mode is RX and packet received, read FIFO
    if (rfm69->_mode == ModeRx && (interrupt_reason & RF_IRQFLAGS2_PAYLOADREADY)) {
        rfm69_read_fifo(rfm69);
        uint8_t rssi_raw;
        read_register(rfm69, REG_RSSIVALUE, &rssi_raw);
        rfm69->_last_rssi = rssi_raw / 2;
    }
}


void interrupt_handler(uint gpio, uint32_t events)
{
    rfm69_handle_interrupt(rfm69_inst);
}


rfm69_error_t rfm69_init(rfm69_t *rfm69)
{   

    rfm69_inst = rfm69;
    // Set the reset pin as an output
    gpio_init(rfm69->reset_pin);
    gpio_set_dir(rfm69->reset_pin, GPIO_OUT);

    // Set the chip select pin as an output
    gpio_init(rfm69->cs_pin);
    gpio_set_dir(rfm69->cs_pin, GPIO_OUT);
    gpio_put(rfm69->cs_pin, 1);

    // Set the interrupt handler
    gpio_set_irq_enabled_with_callback(rfm69->interrupt_pin, GPIO_IRQ_EDGE_RISE, true, &interrupt_handler);

    // Reset the RFM69
    rfm69_reset(rfm69);

    // Set the FIFO threshold
    rfm69_error_t error = rfm69_set_default_fifo_threshold(rfm69);
    if (error != RFM69_OK)
    {
        return error;
    }

    // Enable DAGC
    error = write_register(rfm69, REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0);

    // If high power boost set previously, disable it
    error = write_register(rfm69, REG_TESTPA1, 0x55);
    error = write_register(rfm69, REG_TESTPA2, 0x70);

    // Set the sync word
    uint8_t sync_words[2] = {0x2D, 0xD4};
    error = set_sync_word(rfm69, sync_words, 2);

    error = set_modem_config(rfm69, GFSK_Rb250Fd250);

    // Set the preamble length
    error = rfm69_set_preamble_length(rfm69, 4);

    // Set the default frequency
    error = rfm69_set_frequency(rfm69, 915.0);

    return error;
}

rfm69_error_t rfm69_send(rfm69_t *rfm69, uint8_t *data, uint8_t len)
{

    write_register(rfm69, REG_FIFO, len + 4);
    uint8_t write_buffer[4] = {0xff, 0xff, 0x00, 0x00};
    burst_write(rfm69, REG_FIFO, write_buffer, 4);
    burst_write(rfm69, REG_FIFO, data, len);
    // Set the mode to transmit

    rfm69_error_t err = rfm69_set_mode(rfm69, RF_OPMODE_TRANSMITTER);
}

rfm69_error_t rfm69_print_registers(rfm69_t *rfm69)
{
    for(int i = 0; i < 0x50; i++){
        uint8_t data;
        read_register(rfm69, i, &data);
        printf("Register 0x%02X: 0x%02X\n", i, data);
    }

    uint8_t data;
    read_register(rfm69, 0x58, &data);
    printf("Register 0x%02X: 0x%02X\n", 0x58, data);
    read_register(rfm69, 0x6F, &data);
    printf("Register 0x%02X: 0x%02X\n", 0x6F, data);
    read_register(rfm69, 0x71, &data);
    printf("Register 0x%02X: 0x%02X\n", 0x71, data);
    
}


rfm69_error_t rfm69_set_mode_rx(rfm69_t *rfm69) {
    if (rfm69->_mode != ModeRx){
        if (rfm69->_power >= 18) {
            // If high power boost, return power amp to receive mode
            write_register(rfm69, REG_TESTPA1, 0x55);
            write_register(rfm69, REG_TESTPA2, 0x70);
        }
        // Set interrupt line 0 PayloadReady
        write_register(rfm69, REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01);
        rfm69_set_mode(rfm69, RF_OPMODE_RECEIVER);
        rfm69->_mode = ModeRx;
        return RFM69_OK;
    }   
}

bool rfm69_is_message_available(rfm69_t *rfm69) {
    return rfm69->message_available;
}


rfm69_error_t rfm69_receive(rfm69_t *rfm69, message_t *message) {

    // copy the data from the buffer to the data pointer
    memcpy(message->data, rfm69->_buffer, rfm69->_bufLen);
    message->len = rfm69->_bufLen;
    rfm69->message_available = false;
    message->rssi = rfm69->_last_rssi;
    return RFM69_OK;
}


