#ifndef BAROMETER_H
#define BAROMETER_H

#include <stdint.h>

// I2C address for BMP180
#define BMP180_ADDR 0x77

// Oversampling setting (0 to 3). 0 gives the fastest reading.
#define OSS 0

// Structure to hold calibration coefficients from the BMP180
typedef struct {
    int16_t AC1;
    int16_t AC2;
    int16_t AC3;
    uint16_t AC4;
    uint16_t AC5;
    uint16_t AC6;
    int16_t B1;
    int16_t B2;
    int16_t MB;
    int16_t MC;
    int16_t MD;
} BMP180Calib;

// Structure to hold the processed sensor data (pressure and altitude only)
typedef struct {
    float pressure;    // in hPa
    float altitude;    // in meters (calculated)
} BMP180Data;

// Utility functions
void bpm180_init(void);
static void i2c_open_device(const char *device, int addr);
static int16_t combine_bytes(uint8_t msb, uint8_t lsb);

// BMP180 Operations
void init_bmp180_data(BMP180Calib *bmp_calib, BMP180Data *bmp_data);
void bmp180_read_calibration(void);
int16_t bmp180_read_raw_temperature(void);
int32_t bmp180_read_raw_pressure(void);
void parse_bmp180_data(BMP180Data *data);
void print_bmp180_data(const BMP180Data *data);
BMP180Data* spin_bpm(void);
void terminate_bmp_connection(void);

#endif // BAROMETER_H
