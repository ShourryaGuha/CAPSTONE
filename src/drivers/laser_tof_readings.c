#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>

// Define control registers
const uint16_t REG_ERR_CODE = 0x0000; // System status code
const uint16_t REG_BAT_VLTG = 0x0006; // Input voltage
const uint16_t REG_ADDRESS = 0x0010; // Module address
const uint16_t REG_OFFSET = 0x0012; // Module measure result offset
const uint16_t REG_MEA_START = 0x0020; // Initiate measure
const uint16_t REG_MEA_RESULT = 0x0022; // Measure result
const uint16_t REG_CTRL_LD = 0x01BE; // Laser diode control

int init_sensor() {
}


int read_distance(int file) {
    uint8_t reg = REG_RESULT_RANGE_STATUS + 10; // Register to read from
    uint8_t buf[2];

    if (write(file, &reg, 1) != 1) {
        perror("Failed to write to the i2c bus");
        return -1;
    }

    if (read(file, buf, 2) != 2) {
        perror("Failed to read from the i2c bus");
        return -1;
    }

    int distance = (buf[0] << 8) | buf[1];
    return distance;
}

int main() {
    int file;

    if ((file = open(I2C_BUS, O_RDWR)) < 0) {
        perror("Failed to open the i2c bus");
        return 1;
    }

    if (init_sensor(file) < 0) {
        close(file);
        return 1;
    }

    while (1) {
        int distance = read_distance(file);
        if (distance >= 0) {
            printf("Distance: %d mm\n", distance);
        }
        sleep(1);
    }

    close(file);
    return 0;
}