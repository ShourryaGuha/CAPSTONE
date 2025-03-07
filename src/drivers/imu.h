#ifndef IMU_H
#define IMU_H

#include <stdint.h>

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

// --- Utility functions ---
void i2c_open_device(const char *device, int addr);
int16_t combine_bytes(uint8_t msb, uint8_t lsb);
void init_mpu6050_data(MPU6050Data *MPU6050_data);

// MPU6050 operations
void mpu6050_init(void);
void read_mpu6050_data(uint8_t *buffer);
void parse_mpu6050_data(MPU6050Data *mpu_data, const uint8_t *buffer);
void print_mpu6050_data(const MPU6050Data *mpu_data);
void spin_imu(void);
void terminate_imu_connection(void);

#endif // IMU_H