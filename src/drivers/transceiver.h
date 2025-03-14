#ifndef TRANSCIEVER_H
#define TRANSCIEVER_H

#include <stdint.h>
#include <termios.h>  // For speed_t

// Struct to hold UART data
typedef struct {
    uint16_t distance;  // 2 bytes
    uint16_t servo1;    // 2 bytes
    uint16_t servo2;    // 2 bytes
    uint8_t confirmation; // 1 byte
    uint8_t connected; // 1 byte
} rpi_data;

#define UART_DEVICE "/dev/ttyAMA4"  // Default UART device (can be changed globally)

// General UART initialization function prototype
int uart_init(speed_t baud_rate);

// Function to send data over UART
void send_data(int fd, rpi_data *data);

extern const char *uart_device;  // Declare the UART device as an external variable

#endif // TRANSCIEVER_H
