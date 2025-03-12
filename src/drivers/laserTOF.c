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
#include <signal.h>

// #define DEBUG

volatile sig_atomic_t keep_running = 1;

// 0xFA 0X01 0XFF 0X04 0X01 0X00 0X01 0X00 0X00   single measurement
// 0xFA 0X01 0XFF 0X04 0X01 0X00 0X00 0X00 0XFF  continuous measurement
// 0xFA 0X01 0XFF 0X04 0X00 0X00 0X00 0X00 0XFE  stop command frame

// FILE OPERATION
static int file_open_and_get_descriptor(const char *fname) {
    int fd;

    // fd = open(fname, O_RDWR | O_NONBLOCK);
    fd = open(fname, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd < 0) {
        printf("Could not open file %s...%d\r\n",fname,fd);
    }
    return fd;
}


static int parse_measurement(uint8_t *response, float *distance);
static void signal_handler(int signal);

int Laser_Init(Laser_Device *device) {
    device->fd = file_open_and_get_descriptor(device->device_path);

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(device->fd, &tty))
    {
        printf("Something went wrong while getting port attributes...\r\n");
        close(device->fd);
        exit(EXIT_FAILURE);
    }

    if (cfsetispeed(&tty, BAUD_RATE) < 0 || cfsetospeed(&tty, BAUD_RATE) < 0)
    {
        fprintf(stderr, "Failed to set baud rate: %s\n", strerror(errno));
        close(device->fd);
        exit(EXIT_FAILURE);
    }

    // Set 8N1 (8 bits, no parity, 1 stop bit)
    tty.c_cflag &= ~PARENB;  // No parity
    tty.c_cflag &= ~CSTOPB;  // 1 stop bit
    tty.c_cflag &= ~CSIZE;   // Clear size bits
    tty.c_cflag |= CS8;      // 8 bits
    tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem controls

    // Set input mode (non-canonical, no echo,...)
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable software flow control
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    // Set output mode
    tty.c_oflag &= ~OPOST;

    // Set read timeout
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10; // 1 second timeout

    // cfmakeraw(&tty);

    if (tcsetattr(device->fd, TCSANOW, &tty))
    {
        printf("Something went wrong while setting port attributes...\r\n");
        close(device->fd);
        exit(EXIT_FAILURE);
    }

    // Clear input buffer
    tcflush(device->fd, TCIFLUSH);

    printf("Opened UART device: %s at baud rate: %d\n", device->device_path, 115200);

    device->address = DEFAULT_SLAVE_ADDR;

    return 0; 
}

int Laser_Measure_Single(Laser_Device *device) {
    uint8_t cmd[] = {
        0xFA,  // MsgType
        0x01,  // MsgCode
        0xFF,  // DeviceID default 0xff
        0x04,  // Length
        0x01,  // start measurement 01, stop measurement 00
        0x00,  // Payload   
        0x01,  // single measurement 01, continuous measurement 00
        0x00,  // Payload
        0x00   // Checksum
    };

    if (write(device->fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        fprintf(stderr, "Error: Failed to send measurement command\n");
        return -1;
    }

    usleep(1000);
    // sleep(1);

    uint8_t response[9];
    int bytes_read = read(device->fd, response, sizeof(response));
    if (bytes_read <= 0) {
        #ifdef DEBUG
        fprintf(stderr, "No data received from sensor.\n");
        #endif
        return -1;
    }

    parse_measurement(response, &(device->distance));

    //print raw value for debugging
    // for(uint8_t i=0; i < bytes_read; i++) {
    //     printf("%02X ", response[i]);
    // }

    //print distance in metres
    #ifdef DEBUG
        printf("Distance: %.2f m\n", device->distance);
    #endif

    return 0;
}

/**
 * Handle SIGINT (Ctrl+C)
 */
static void signal_handler(int signal) {
    if (signal == SIGINT) {
        printf("\nStopping measurement...\n");
        keep_running = 0;
    }
}


int spin_laser(Laser_Device *device){
    uint8_t response[9];
    int bytes_read = read(device->fd, response, sizeof(response));
    if (bytes_read <= 0) {
        #ifdef DEBUG
        fprintf(stderr, "No data received from sensor.\n");
        #endif
        return -1;
    }
    parse_measurement(response, &(device->distance));
    //print distance in metres
    #ifdef DEBUG
        printf("Distance: %.2f m\n", device->distance);
    #endif
}

int Laser_Measure_Continuous(Laser_Device *device) {
    uint8_t cmd[] = {
        0xFA,  // MsgType
        0x01,  // MsgCode
        0xFF,  // DeviceID default 0xff
        0x04,  // Length
        0x01,  // start measurement 01, stop measurement 00
        0x00,  // Payload   
        0x01,  // single measurement 01, continuous measurement 00
        0x00,  // Payload
        0x00   // Checksum
    };

    // Set up signal handler for Ctrl+C
    signal(SIGINT, signal_handler);

    if (write(device->fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        fprintf(stderr, "Error: Failed to send measurement command\n");
        return -1;
    }

    // sleep(1);
    // usleep(1000);
}

int Laser_stop_continuous(Laser_Device *device) {
    uint8_t cmd[] = {
        0xFA,  // MsgType
        0x01,  // MsgCode
        0xFF,  // DeviceID default 0xff
        0x04,  // Length
        0x00,  // start measurement 01, stop measurement 00
        0x00,  // Payload
        0x00,  // Payload
        0x00,  // Payload
        0xFE   // Checksum
    };

    if (write(device->fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        fprintf(stderr, "Error: Failed to send stop command \n");
        return -1;
    }

}


static int parse_measurement(uint8_t *response, float *distance) {
    uint16_t check_sum = 0;
    for(int i = 0; i < 8; i++) {
        check_sum += response[i];
    }
    // Check if check_sum is equal to the last 8 bits of response[9]
    if ((check_sum & 0xFF) != response[8]) {
        printf("Checksum is invalid.\n");
        return -1;
    }

    uint16_t distance_decimeters = (response[7] << 8) | response[6];
    *distance = distance_decimeters / 10.0f;
    return 0;
}