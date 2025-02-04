#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <math.h>
#include <string.h>

// I2C address for MPU6050
#define MPU6050_ADDR 0x68

typedef struct {
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
} MPU6050Data;

static MPU6050Data mpu_data;

// --- Utility functions ---
int i2c_open_device(const char *device, int addr) {
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("Error opening I2C device");
        exit(1);
    }
    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        perror("Error setting I2C slave address");
        exit(1);
    }
    return fd;
}

int16_t combine_bytes(uint8_t msb, uint8_t lsb) {
    return (int16_t)((msb << 8) | lsb);
}

void init_mpu6050_data(MPU6050Data *MPU6050_data) {
    memset(MPU6050_data, 0, sizeof(MPU6050Data));
}

void mpu6050_init(int fd) {
    uint8_t data[2] = {0x6B, 0x00}; // Power management register: wake up
    if (write(fd, data, 2) != 2) {
        perror("MPU6050 initialization failed");
        exit(1);
    }
    init_mpu6050_data(&mpu_data);
}

int read_mpu6050_data(int fd, uint8_t *buffer) {
    uint8_t reg = 0x3B;
    if (write(fd, &reg, 1) != 1) {
        perror("Failed to set MPU6050 register pointer");
        return -1;
    }
    if (read(fd, buffer, 14) != 14) {
        perror("Failed to read MPU6050 data");
        return -1;
    }
    return 0;
}

void parse_mpu6050_data(MPU6050Data *mpu_data, const uint8_t *buffer) {
    int16_t raw_ax = combine_bytes(buffer[0], buffer[1]);
    int16_t raw_ay = combine_bytes(buffer[2], buffer[3]);
    int16_t raw_az = combine_bytes(buffer[4], buffer[5]);
    int16_t raw_gx = combine_bytes(buffer[8], buffer[9]);
    int16_t raw_gy = combine_bytes(buffer[10], buffer[11]);
    int16_t raw_gz = combine_bytes(buffer[12], buffer[13]);

    mpu_data->ax = raw_ax / 16384.0;
    mpu_data->ay = raw_ay / 16384.0;
    mpu_data->az = raw_az / 16384.0;
    mpu_data->gx = raw_gx / 131.0;
    mpu_data->gy = raw_gy / 131.0;
    mpu_data->gz = raw_gz / 131.0;
}

void print_mpu6050_data(const MPU6050Data *mpu_data) {
    printf("MPU6050:\n");
    printf("  Acceleration: x=%.2f g, y=%.2f g, z=%.2f g\n", mpu_data->ax, mpu_data->ay, mpu_data->az);
    printf("  Gyroscope:    x=%.2f °/s, y=%.2f °/s, z=%.2f °/s\n", mpu_data->gx, mpu_data->gy, mpu_data->gz);
}

void spin(int fd_mpu) {
    uint8_t buffer[14];
    if (read_mpu6050_data(fd_mpu, buffer) == 0) {
        parse_mpu6050_data(&mpu_data, buffer);
        print_mpu6050_data(&mpu_data);
    }
}

int main() {
    const char *i2c_device = "/dev/i2c-1";
    int fd_mpu = i2c_open_device(i2c_device, MPU6050_ADDR);

    mpu6050_init(fd_mpu);

    while (1) {
        spin(fd_mpu);
        usleep(100000);
    }

    close(fd_mpu);
    return 0;
}
