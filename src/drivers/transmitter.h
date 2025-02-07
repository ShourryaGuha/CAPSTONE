#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <stdint.h>

// Pin Definitions for the NRF24L01+ module
#define CE_PIN      8         // Chip Enable pin (GPIO 8)
#define CSN_PIN     26        // Chip Select Not pin (GPIO 26)
#define SPI_CHANNEL 0         // SPI channel 0 (/dev/spidev0.0)
#define SPI_SPEED   500000    // SPI clock speed (500 kHz)

// NRF24L01+ Basic Commands
#define NRF_W_REGISTER    0x20  // Command for writing to a register
#define NRF_W_TX_PAYLOAD  0xA0  // Command for writing TX payload
#define NRF_FLUSH_TX      0xE1  // Command to flush TX FIFO

// Structure for sensor data (distance and servo angles)
typedef struct __attribute__((packed)) {
    uint16_t distance;   // Distance sensor reading (16-bit)
    uint16_t servo1;     // Servo 1 angle (16-bit)
    uint16_t servo2;     // Servo 2 angle (16-bit)
} SensorData;

// Function Prototypes
void nrf24_init();                          // Initializes the NRF24L01+ in TX mode
void nrf24_write_register(uint8_t reg, uint8_t value);  // Writes a single byte to a register
void nrf24_write_register_multi(uint8_t reg, uint8_t *data, uint8_t len); // Writes multiple bytes
void nrf24_flush_tx();                      // Flushes the TX FIFO
void nrf24_transmit_payload(uint8_t *payload, uint8_t len); // Transmits the payload
void terminate_transmitter(); // Terminates the transmitter

#endif // TRANSMITTER_H