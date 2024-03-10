#ifndef RFM69_H
#define RFM69_H

#include "pico/stdlib.h"
#include "rfm69_registers.h"
#include "hardware/spi.h"

// The crystal oscillator frequency of the RF69 module
#define RF69_FXOSC 32000000.0

// The Frequency Synthesizer step = RF69_FXOSC / 2^^19
#define RF69_FSTEP (RF69_FXOSC / 524288)

// This is the maximum number of interrupts the driver can support
// Most Arduinos can handle 2, Megas can handle more
#define RF69_NUM_INTERRUPTS 3

// This is the bit in the SPI address that marks it as a write
#define RF69_SPI_WRITE_MASK 0x80

// Max number of octets the RF69 Rx and Tx FIFOs can hold
#define RF69_FIFO_SIZE 66

// Maximum encryptable payload length the RF69 can support
#define RF69_MAX_ENCRYPTABLE_PAYLOAD_LEN 64

// The length of the headers we add.
// The headers are inside the RF69's payload and are therefore encrypted if encryption is enabled
#define RF69_HEADER_LEN 4

// This is the maximum message length that can be supported by this driver. Limited by
// the size of the FIFO, since we are unable to support on-the-fly filling and emptying
// of the FIFO.
// Can be pre-defined to a smaller size (to save SRAM) prior to including this header
// Here we allow for 4 bytes of address and header and payload to be included in the 64 byte encryption limit.
// the one byte payload length is not encrpyted
#ifndef RF69_MAX_MESSAGE_LEN
#define RF69_MAX_MESSAGE_LEN (RF69_MAX_ENCRYPTABLE_PAYLOAD_LEN - RF69_HEADER_LEN)
#endif

// RH_RF69_REG_37_PACKETCONFIG1
#define RH_RF69_PACKETCONFIG1_PACKETFORMAT_VARIABLE         0x80
#define RH_RF69_PACKETCONFIG1_DCFREE                        0x60
#define RH_RF69_PACKETCONFIG1_DCFREE_NONE                   0x00
#define RH_RF69_PACKETCONFIG1_DCFREE_MANCHESTER             0x20
#define RH_RF69_PACKETCONFIG1_DCFREE_WHITENING              0x40
#define RH_RF69_PACKETCONFIG1_DCFREE_RESERVED               0x60
#define RH_RF69_PACKETCONFIG1_CRC_ON                        0x10
#define RH_RF69_PACKETCONFIG1_CRCAUTOCLEAROFF               0x08
#define RH_RF69_PACKETCONFIG1_ADDRESSFILTERING              0x06
#define RH_RF69_PACKETCONFIG1_ADDRESSFILTERING_NONE         0x00
#define RH_RF69_PACKETCONFIG1_ADDRESSFILTERING_NODE         0x02
#define RH_RF69_PACKETCONFIG1_ADDRESSFILTERING_NODE_BC      0x04
#define RH_RF69_PACKETCONFIG1_ADDRESSFILTERING_RESERVED     0x06


typedef enum
{
    ModeInitialising = 0, ///< Transport is initialising. Initial default value until init() is called..
    ModeSleep,            ///< Transport hardware is in low power sleep mode (if supported)
    ModeIdle,             ///< Transport is idle.
    ModeTx,               ///< Transport is in the process of transmitting a message.
    ModeRx,               ///< Transport is in the process of receiving a message.
    ModeCad               ///< Transport is in the process of detecting channel activity (if supported)
} Mode;

/// Choices for setModemConfig() for a selected subset of common
/// modulation types, and data rates. If you need another configuration,
/// use the register calculator.  and call setModemRegisters() with your
/// desired settings.
/// These are indexes into MODEM_CONFIG_TABLE. We strongly recommend you use these symbolic
/// definitions and not their integer equivalents: its possible that new values will be
/// introduced in later versions (though we will try to avoid it).
/// CAUTION: some of these configurations do not work corectly and are marked as such.
typedef enum
{
    FSK_Rb2Fd5 = 0,   ///< FSK, Whitening, Rb = 2kbs,    Fd = 5kHz
    FSK_Rb2_4Fd4_8,   ///< FSK, Whitening, Rb = 2.4kbs,  Fd = 4.8kHz
    FSK_Rb4_8Fd9_6,   ///< FSK, Whitening, Rb = 4.8kbs,  Fd = 9.6kHz
    FSK_Rb9_6Fd19_2,  ///< FSK, Whitening, Rb = 9.6kbs,  Fd = 19.2kHz
    FSK_Rb19_2Fd38_4, ///< FSK, Whitening, Rb = 19.2kbs, Fd = 38.4kHz
    FSK_Rb38_4Fd76_8, ///< FSK, Whitening, Rb = 38.4kbs, Fd = 76.8kHz
    FSK_Rb57_6Fd120,  ///< FSK, Whitening, Rb = 57.6kbs, Fd = 120kHz
    FSK_Rb125Fd125,   ///< FSK, Whitening, Rb = 125kbs,  Fd = 125kHz
    FSK_Rb250Fd250,   ///< FSK, Whitening, Rb = 250kbs,  Fd = 250kHz
    FSK_Rb55555Fd50,  ///< FSK, Whitening, Rb = 55555kbs,Fd = 50kHz for RFM69 lib compatibility

    GFSK_Rb2Fd5,       ///< GFSK, Whitening, Rb = 2kbs,    Fd = 5kHz
    GFSK_Rb2_4Fd4_8,   ///< GFSK, Whitening, Rb = 2.4kbs,  Fd = 4.8kHz
    GFSK_Rb4_8Fd9_6,   ///< GFSK, Whitening, Rb = 4.8kbs,  Fd = 9.6kHz
    GFSK_Rb9_6Fd19_2,  ///< GFSK, Whitening, Rb = 9.6kbs,  Fd = 19.2kHz
    GFSK_Rb19_2Fd38_4, ///< GFSK, Whitening, Rb = 19.2kbs, Fd = 38.4kHz
    GFSK_Rb38_4Fd76_8, ///< GFSK, Whitening, Rb = 38.4kbs, Fd = 76.8kHz
    GFSK_Rb57_6Fd120,  ///< GFSK, Whitening, Rb = 57.6kbs, Fd = 120kHz
    GFSK_Rb125Fd125,   ///< GFSK, Whitening, Rb = 125kbs,  Fd = 125kHz
    GFSK_Rb250Fd250,   ///< GFSK, Whitening, Rb = 250kbs,  Fd = 250kHz
    GFSK_Rb55555Fd50,  ///< GFSK, Whitening, Rb = 55555kbs,Fd = 50kHz

    OOK_Rb1Bw1,       ///< OOK, Whitening, Rb = 1kbs,    Rx Bandwidth = 1kHz.
    OOK_Rb1_2Bw75,    ///< OOK, Whitening, Rb = 1.2kbs,  Rx Bandwidth = 75kHz.
    OOK_Rb2_4Bw4_8,   ///< OOK, Whitening, Rb = 2.4kbs,  Rx Bandwidth = 4.8kHz.
    OOK_Rb4_8Bw9_6,   ///< OOK, Whitening, Rb = 4.8kbs,  Rx Bandwidth = 9.6kHz.
    OOK_Rb9_6Bw19_2,  ///< OOK, Whitening, Rb = 9.6kbs,  Rx Bandwidth = 19.2kHz.
    OOK_Rb19_2Bw38_4, ///< OOK, Whitening, Rb = 19.2kbs, Rx Bandwidth = 38.4kHz.
    OOK_Rb32Bw64,     ///< OOK, Whitening, Rb = 32kbs,   Rx Bandwidth = 64kHz.

    //	Test,
} ModemConfigChoice;

// This is the default node address,
#define RF69_DEFAULT_NODE_ADDRESS 0

// You can define the following macro (either by editing here or by passing it as a compiler definition
// to change the default value of the ishighpowermodule argument to setTxPower to true
//
// #define RFM69_HW
#ifdef RFM69_HW
#define RF69_DEFAULT_HIGHPOWER true
#else
#define RF69_DEFAULT_HIGHPOWER false
#endif


// Define this to include Serial printing in diagnostic routines
#define RF69_HAVE_SERIAL

typedef struct
{
    uint8_t reg_02; ///< Value for register RH_RF69_REG_02_DATAMODUL
    uint8_t reg_03; ///< Value for register RH_RF69_REG_03_BITRATEMSB
    uint8_t reg_04; ///< Value for register RH_RF69_REG_04_BITRATELSB
    uint8_t reg_05; ///< Value for register RH_RF69_REG_05_FDEVMSB
    uint8_t reg_06; ///< Value for register RH_RF69_REG_06_FDEVLSB
    uint8_t reg_19; ///< Value for register RH_RF69_REG_19_RXBW
    uint8_t reg_1a; ///< Value for register RH_RF69_REG_1A_AFCBW
    uint8_t reg_37; ///< Value for register RH_RF69_REG_37_PACKETCONFIG1
} ModemConfig;

// These are indexed by the values of ModemConfigChoice
// Stored in flash (program) memory to save SRAM
// It is important to keep the modulation index for FSK between 0.5 and 10
// modulation index = 2 * Fdev / BR
// Note that I have not had much success with FSK with Fd > ~5
// You have to construct these by hand, using the data from the RF69 Datasheet :-(
// or use the SX1231 starter kit software (Ctl-Alt-N to use that without a connected radio)
#define CONFIG_FSK (RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00)
#define CONFIG_GFSK (RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_01)
#define CONFIG_OOK (RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_OOK | RF_DATAMODUL_MODULATIONSHAPING_00)

// Choices for RH_RF69_REG_37_PACKETCONFIG1:
#define CONFIG_NOWHITE (RH_RF69_PACKETCONFIG1_PACKETFORMAT_VARIABLE | RH_RF69_PACKETCONFIG1_DCFREE_NONE | RH_RF69_PACKETCONFIG1_CRC_ON | RH_RF69_PACKETCONFIG1_ADDRESSFILTERING_NONE)
#define CONFIG_WHITE (RH_RF69_PACKETCONFIG1_PACKETFORMAT_VARIABLE | RH_RF69_PACKETCONFIG1_DCFREE_WHITENING | RH_RF69_PACKETCONFIG1_CRC_ON | RH_RF69_PACKETCONFIG1_ADDRESSFILTERING_NONE)
#define CONFIG_MANCHESTER (RH_RF69_PACKETCONFIG1_PACKETFORMAT_VARIABLE | RH_RF69_PACKETCONFIG1_DCFREE_MANCHESTER | RH_RF69_PACKETCONFIG1_CRC_ON | RH_RF69_PACKETCONFIG1_ADDRESSFILTERING_NONE)

static const ModemConfig MODEM_CONFIG_TABLE[] =
    {
        //  02,        03,   04,   05,   06,   19,   1a,  37
        // FSK, No Manchester, no shaping, whitening, CRC, no address filtering
        // AFC BW == RX BW == 2 x bit rate
        // Low modulation indexes of ~ 1 at slow speeds do not seem to work very well. Choose MI of 2.
        {CONFIG_FSK, 0x3e, 0x80, 0x00, 0x52, 0xf4, 0xf4, CONFIG_WHITE}, // FSK_Rb2Fd5
        {CONFIG_FSK, 0x34, 0x15, 0x00, 0x4f, 0xf4, 0xf4, CONFIG_WHITE}, // FSK_Rb2_4Fd4_8
        {CONFIG_FSK, 0x1a, 0x0b, 0x00, 0x9d, 0xf4, 0xf4, CONFIG_WHITE}, // FSK_Rb4_8Fd9_6

        {CONFIG_FSK, 0x0d, 0x05, 0x01, 0x3b, 0xf4, 0xf4, CONFIG_WHITE}, // FSK_Rb9_6Fd19_2
        {CONFIG_FSK, 0x06, 0x83, 0x02, 0x75, 0xf3, 0xf3, CONFIG_WHITE}, // FSK_Rb19_2Fd38_4
        {CONFIG_FSK, 0x03, 0x41, 0x04, 0xea, 0xf2, 0xf2, CONFIG_WHITE}, // FSK_Rb38_4Fd76_8

        {CONFIG_FSK, 0x02, 0x2c, 0x07, 0xae, 0xe2, 0xe2, CONFIG_WHITE}, // FSK_Rb57_6Fd120
        {CONFIG_FSK, 0x01, 0x00, 0x08, 0x00, 0xe1, 0xe1, CONFIG_WHITE}, // FSK_Rb125Fd125
        {CONFIG_FSK, 0x00, 0x80, 0x10, 0x00, 0xe0, 0xe0, CONFIG_WHITE}, // FSK_Rb250Fd250
        {CONFIG_FSK, 0x02, 0x40, 0x03, 0x33, 0x42, 0x42, CONFIG_WHITE}, // FSK_Rb55555Fd50

        //  02,        03,   04,   05,   06,   19,   1a,  37
        // GFSK (BT=1.0), No Manchester, whitening, CRC, no address filtering
        // AFC BW == RX BW == 2 x bit rate
        {CONFIG_GFSK, 0x3e, 0x80, 0x00, 0x52, 0xf4, 0xf5, CONFIG_WHITE}, // GFSK_Rb2Fd5
        {CONFIG_GFSK, 0x34, 0x15, 0x00, 0x4f, 0xf4, 0xf4, CONFIG_WHITE}, // GFSK_Rb2_4Fd4_8
        {CONFIG_GFSK, 0x1a, 0x0b, 0x00, 0x9d, 0xf4, 0xf4, CONFIG_WHITE}, // GFSK_Rb4_8Fd9_6

        {CONFIG_GFSK, 0x0d, 0x05, 0x01, 0x3b, 0xf4, 0xf4, CONFIG_WHITE}, // GFSK_Rb9_6Fd19_2
        {CONFIG_GFSK, 0x06, 0x83, 0x02, 0x75, 0xf3, 0xf3, CONFIG_WHITE}, // GFSK_Rb19_2Fd38_4
        {CONFIG_GFSK, 0x03, 0x41, 0x04, 0xea, 0xf2, 0xf2, CONFIG_WHITE}, // GFSK_Rb38_4Fd76_8

        {CONFIG_GFSK, 0x02, 0x2c, 0x07, 0xae, 0xe2, 0xe2, CONFIG_WHITE}, // GFSK_Rb57_6Fd120
        {CONFIG_GFSK, 0x01, 0x00, 0x08, 0x00, 0xe1, 0xe1, CONFIG_WHITE}, // GFSK_Rb125Fd125
        {CONFIG_GFSK, 0x00, 0x80, 0x10, 0x00, 0xe0, 0xe0, CONFIG_WHITE}, // GFSK_Rb250Fd250
        {CONFIG_GFSK, 0x02, 0x40, 0x03, 0x33, 0x42, 0x42, CONFIG_WHITE}, // GFSK_Rb55555Fd50

        //  02,        03,   04,   05,   06,   19,   1a,  37
        // OOK, No Manchester, no shaping, whitening, CRC, no address filtering
        // with the help of the SX1231 configuration program
        // AFC BW == RX BW
        // All OOK configs have the default:
        // Threshold Type: Peak
        // Peak Threshold Step: 0.5dB
        // Peak threshiold dec: ONce per chip
        // Fixed threshold: 6dB
        {CONFIG_OOK, 0x7d, 0x00, 0x00, 0x10, 0x88, 0x88, CONFIG_WHITE}, // OOK_Rb1Bw1
        {CONFIG_OOK, 0x68, 0x2b, 0x00, 0x10, 0xf1, 0xf1, CONFIG_WHITE}, // OOK_Rb1_2Bw75
        {CONFIG_OOK, 0x34, 0x15, 0x00, 0x10, 0xf5, 0xf5, CONFIG_WHITE}, // OOK_Rb2_4Bw4_8
        {CONFIG_OOK, 0x1a, 0x0b, 0x00, 0x10, 0xf4, 0xf4, CONFIG_WHITE}, // OOK_Rb4_8Bw9_6
        {CONFIG_OOK, 0x0d, 0x05, 0x00, 0x10, 0xf3, 0xf3, CONFIG_WHITE}, // OOK_Rb9_6Bw19_2
        {CONFIG_OOK, 0x06, 0x83, 0x00, 0x10, 0xf2, 0xf2, CONFIG_WHITE}, // OOK_Rb19_2Bw38_4
        {CONFIG_OOK, 0x03, 0xe8, 0x00, 0x10, 0xe2, 0xe2, CONFIG_WHITE}, // OOK_Rb32Bw64

        //    { CONFIG_FSK,  0x68, 0x2b, 0x00, 0x52, 0x55, 0x55, CONFIG_WHITE}, // works: Rb1200 Fd 5000 bw10000, DCC 400
        //    { CONFIG_FSK,  0x0c, 0x80, 0x02, 0x8f, 0x52, 0x52, CONFIG_WHITE}, // works 10/40/80
        //    { CONFIG_FSK,  0x0c, 0x80, 0x02, 0x8f, 0x53, 0x53, CONFIG_WHITE}, // works 10/40/40

};

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
    uint interrupt_pin;
    uint reset_pin;
    Mode _mode;
    uint _power;
    uint8_t _buffer[60];
    uint8_t _bufLen;
    bool message_available;
} rfm69_t;

typedef struct {
    uint8_t len;
    uint8_t data[RF69_MAX_MESSAGE_LEN];
} message_t;



uint8_t rfm69_get_revision(rfm69_t *rfm69, uint8_t *revision);

// Reset the RFM69 module
void rfm69_reset(rfm69_t *rfm69);

// Set the operation mode
rfm69_error_t rfm69_set_mode(rfm69_t *rfm69, uint8_t mode);

rfm69_error_t rfm69_init(rfm69_t *rfm69);

rfm69_error_t rfm69_send(rfm69_t *rfm69, uint8_t *data, uint8_t len);

rfm69_error_t rfm69_set_tx_power(rfm69_t *rfm69, uint8_t tx_power, bool is_high_power_module);

rfm69_error_t rfm69_print_registers(rfm69_t *rfm69);

rfm69_error_t rfm69_set_mode_tx(rfm69_t *rfm69);

rfm69_error_t rfm69_set_mode_rx(rfm69_t *rfm69);

rfm69_error_t rfm69_receive(rfm69_t *rfm69, message_t *message);

bool rfm69_is_message_available(rfm69_t *rfm69);

#endif // RFM69_H