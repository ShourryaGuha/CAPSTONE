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

// FILE OPERATION
static int file_open_and_get_descriptor(const char *fname) {
    int fd;

    fd = open(fname, O_RDWR | O_NONBLOCK);
    if(fd < 0) {
        printf("Could not open file %s...%d\r\n",fname,fd);
    }
    return fd;
}

int Laser_Init(Laser_Device *device) {
    device->fd = file_open_and_get_descriptor(device->device_path);

    struct termios tty;
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

    cfmakeraw(&tty);

    if (tcsetattr(device->fd, TCSANOW, &tty))
    {
        printf("Something went wrong while setting port attributes...\r\n");
        close(device->fd);
        exit(EXIT_FAILURE);
    }

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

    if (write(device->fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        fprintf(stderr, "Error: Failed to send measurement command\n");
        return -1;
    }

    sleep(1);

    uint8_t response[9];
    int bytes_read = read(device->fd, response, sizeof(response));
    if (bytes_read <= 0) {
        fprintf(stderr, "No data received from sensor.\n");
        return -1;
    }

    for(uint8_t i=0; i < bytes_read; i++) {
        printf("%02X ", response[i]);
    }

    return 0;
}