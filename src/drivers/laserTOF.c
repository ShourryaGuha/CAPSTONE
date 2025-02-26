#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include "laserTOF.h"

// Constants
#define UART2_DEVICE "/dev/ttyAMA2"   // J1 on pi
#define UART3_DEVICE "/dev/ttyAMA3"     // J3 on pi


//0xFA 0X01 0XFF 0X04 0X01 0X00 0X01 0X00 0X00   single measurement
// 0xFA 0X01 0XFF 0X04 0X01 0X00 0X00 0X00 0XFF  continuous measurement
// 0xFA 0X01 0XFF 0X04 0X00 0X00 0X00 0X00 0XFE  stop command frame

// // Structure for each laser sensor
// typedef struct
// {
//     int fd;                         // File descriptor for serial port
//     uint8_t address;                // Device address
//     const char *device_path;        // UART device path
//     // MeasurementBuffer measurements; // Circular buffer of measurements
//     // float last_valid_distance;      // Last valid measurement
//     // time_t last_measurement_time;   // Timestamp of last measurement
//     // Laser_Status last_status;       // Last status read
//     float distance;  //measured distance
// } Laser_Device;

// Initialize a laser rangefinder
int Laser_Init(Laser_Device *device) {
    struct termios tty;
    // Open serial port
    device->fd = open(device->device_path, O_RDWR | O_NOCTTY | O_NDELAY);
    if (device->fd < 0)
    {
        fprintf(stderr, "Failed to open serial port %s: %s\n", device->device_path, strerror(errno));
        return -1;
    }

    // Get current serial port settings
    if (tcgetattr(device->fd, &tty) != 0)
    {
        fprintf(stderr, "Failed to get serial port attributes: %s\n", strerror(errno));
        close(device->fd);
        return -1;
    }

    // Clear all settings
    memset(&tty, 0, sizeof(tty));

    // Set input and output baud rate
    if (cfsetispeed(&tty, BAUD_RATE) < 0 || cfsetospeed(&tty, BAUD_RATE) < 0)
    {
        fprintf(stderr, "Failed to set baud rate: %s\n", strerror(errno));
        close(device->fd);
        return -1;
    }

    // Control modes
    tty.c_cflag |= CS8;     // 8 data bits
    tty.c_cflag &= ~PARENB; // No parity
    tty.c_cflag &= ~CSTOPB; // 1 stop bit
    tty.c_cflag |= CLOCAL;  // Ignore modem status lines
    tty.c_cflag |= CREAD;   // Enable receiver

    // Input modes
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);

    // Output modes
    tty.c_oflag &= ~OPOST; // Raw output

    // Local modes
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    // Read settings
    tty.c_cc[VMIN] = 0;  // Return even if no chars received
    tty.c_cc[VTIME] = 10; // Wait up to 0.5 seconds

    // Flush port
    tcflush(device->fd, TCIOFLUSH);

    // Apply settings
    if (tcsetattr(device->fd, TCSANOW, &tty) != 0)
    {
        fprintf(stderr, "Failed to set serial port attributes: %s\n", strerror(errno));
        close(device->fd);
        return -1;
    }

    // Set blocking mode
    fcntl(device->fd, F_SETFL, 0);

    device->address = DEFAULT_SLAVE_ADDR;

    return 0;
    
}

int Laser_Measure_Single(Laser_Device *device, Laser_Mode mode, float *distance) {
    uint8_t cmd[] = {
        0xFA,  // MsgType
        0x01,  // MsgCode
        0xFF,  // DeviceID default 0xff
        0x04,  // Length
        0x01,  // start measurement 01, stop measurement 00
        0x00,  // Payload
        0x00,  // single measurement 01, continuous measurement 00
        0x00,  // Payload
        0xFF   // Checksum
    };

    // Send measurement start command
    if (write(device->fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        fprintf(stderr, "Error: Failed to send measurement command\n");
        return -1;
    }

    // Read response
    uint8_t response[12];
    int bytes_read = read(device->fd, response, sizeof(response));

    // Print raw response
    fprintf(stderr, "Raw Sensor Response: ");
    for (int i = 0; i < bytes_read; i++) {
        fprintf(stderr, "%02X ", response[i]);
    }
    fprintf(stderr, "\n");
}