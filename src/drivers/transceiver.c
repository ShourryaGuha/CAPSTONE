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
} rpi_data;

// Global UART device variable
#define UART_DEVICE "/dev/serial0"  // Default UART device (can be changed globally)

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
    uint8_t buffer[7];
    
    // Pack the data into the buffer
    buffer[0] = (uint8_t)(data->distance >> 8);  // High byte of distance
    buffer[1] = (uint8_t)(data->distance & 0xFF); // Low byte of distance
    buffer[2] = (uint8_t)(data->servo1 >> 8);    // High byte of servo1
    buffer[3] = (uint8_t)(data->servo1 & 0xFF);  // Low byte of servo1
    buffer[4] = (uint8_t)(data->servo2 >> 8);    // High byte of servo2
    buffer[5] = (uint8_t)(data->servo2 & 0xFF);  // Low byte of servo2
    buffer[6] = data->confirmation;              // Confirmation integer (1-3)

    // Send the data
    write(fd, buffer, sizeof(buffer));
    printf("Data Sent: Distance: %d, Servo1: %d, Servo2: %d, Confirmation: %d\n", 
            data->distance, data->servo1, data->servo2, data->confirmation);
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
