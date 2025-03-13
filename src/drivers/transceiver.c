#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <stdint.h>

// Struct to hold data
typedef struct {
    uint16_t distance;  // 2 bytes
    uint16_t servo1;    // 2 bytes
    uint16_t servo2;    // 2 bytes
    uint8_t confirmation; // 1 byte
    uint8_t connected; // 1 byte
} rpi_data;

// Global UART device variable
#define UART_DEVICE "/dev/ttyAMA4"  // Default UART device (can be changed globally)

// General UART initialization function
int uart_init(speed_t baud_rate) {
    int fd = open(UART_DEVICE, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == -1) {
        perror("Unable to open UART");
        exit(1);
    }

    struct termios options;
    tcgetattr(fd, &options);

    // Set baud rate
    cfsetispeed(&options, baud_rate);  // Set input baud rate
    cfsetospeed(&options, baud_rate);  // Set output baud rate

    // 8N1 - 8 data bits, No parity, 1 stop bit
    options.c_cflag &= ~PARENB;    // No parity
    options.c_cflag &= ~CSTOPB;    // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;        // 8 data bits
    options.c_cflag |= CREAD | CLOCAL; // Enable receiver and local mode

    // Apply settings
    tcsetattr(fd, TCSANOW, &options);

    return fd;
}

// Function to send data over UART
void send_data(int fd, rpi_data *data) {
    char temp[32] = {0};   // Temporary buffer for the formatted string
    // Format the struct values into a comma separated string (without newline)
    // Using %hu for 16-bit unsigned and %hhu for 8-bit unsigned values.
    snprintf(temp, sizeof(temp), "%hu,%hu,%hu,%hhu,%hhu", 
             data->distance, data->servo1, data->servo2, 
             data->confirmation, data->connected);

    int len = strlen(temp);
    // Create the final 32-byte buffer
    char buffer[32];

    // Fill the buffer with spaces using a for loop
    for (int i = 0; i < sizeof(buffer); i++) {
        buffer[i] = ' ';
    }
    
    // Copy the formatted string into the beginning of the buffer
    memcpy(buffer, temp, len);
    // Optionally, set the last byte to a newline as a packet delimiter
    buffer[31] = '\n';

    // Send the full 32 bytes over UART
    write(fd, buffer, sizeof(buffer));

    // Debug: Print the sent packet in hexadecimal for clarity
    printf("Sent data (32 bytes): ");
    for (int i = 0; i < sizeof(buffer); i++) {
        printf("%02X ", (unsigned char)buffer[i]);
    }
    printf("\n");

    usleep(10);
}

// int main() {
//     // Initialize UART (use the global device and baud rate)
//     int uart_fd = uart_init(B9600);  // Set baud rate to 9600

//     // Prepare test data
//     uart_data_t data;
//     data.distance = 1234;  // Example distance
//     data.servo1 = 1500;    // Example servo1
//     data.servo2 = 1500;    // Example servo2
//     data.confirmation = 2; // Example confirmation value

//     // Send the data
//     send_data(uart_fd, &data);

//     // Close the UART
//     close(uart_fd);

//     return 0;
// }
