#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <math.h>
#include <string.h>

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

static BMP180Calib bmp_calib;
static BMP180Data bmp_data;
static int bmp_fd = -1;

// --- Utility functions ---

// Open the I2C device and set the slave address
void i2c_open_device(const char *device, int addr) {
    bmp_fd = open(device, O_RDWR);
    if (bmp_fd < 0) {
        perror("Error opening I2C device");
        exit(1);
    }
    if (ioctl(bmp_fd, I2C_SLAVE, addr) < 0) {
        perror("Error setting I2C slave address");
        close(bmp_fd);
        exit(1);
    }
}

// Combine two 8-bit values into a signed 16-bit value
int16_t combine_bytes(uint8_t msb, uint8_t lsb) {
    return (int16_t)((msb << 8) | lsb);
}

// Initialize BPM180 data structs
void init_bpm180_data(BMP180Calib *bmp_calib, BMP180Data *bmp_data) {
    memset(bmp_calib, 0, sizeof(BMP180Calib));
    memset(bmp_data, 0, sizeof(BMP180Data));
}

// Initialize BPM180
void bpm180_init() {
    const char *i2c_device = "/dev/i2c-1";
    i2c_open_device(i2c_device, BMP180_ADDR);
    init_bpm180_data(&bmp_calib, &bmp_data);
    bmp180_read_calibration();
}

// --- BMP180 Calibration Functions ---

// Read 22 bytes of calibration data from registers 0xAA to 0xBF
void bmp180_read_calibration() {
    uint8_t reg = 0xAA;
    uint8_t calib_data[22];
    
    // Set the calibration data register pointer
    if (write(bmp_fd, &reg, 1) != 1) {
        perror("BMP180: Failed to set calibration register pointer");
        exit(1);
    }
    // Read 22 bytes of calibration data
    if (read(bmp_fd, calib_data, 22) != 22) {
        perror("BMP180: Failed to read calibration data");
        exit(1);
    }
    
    // Parse calibration data
    bmp_calib.AC1 = combine_bytes(calib_data[0], calib_data[1]);
    bmp_calib.AC2 = combine_bytes(calib_data[2], calib_data[3]);
    bmp_calib.AC3 = combine_bytes(calib_data[4], calib_data[5]);
    bmp_calib.AC4 = (calib_data[6] << 8) | calib_data[7];
    bmp_calib.AC5 = (calib_data[8] << 8) | calib_data[9];
    bmp_calib.AC6 = (calib_data[10] << 8) | calib_data[11];
    bmp_calib.B1  = combine_bytes(calib_data[12], calib_data[13]);
    bmp_calib.B2  = combine_bytes(calib_data[14], calib_data[15]);
    bmp_calib.MB  = combine_bytes(calib_data[16], calib_data[17]);
    bmp_calib.MC  = combine_bytes(calib_data[18], calib_data[19]);
    bmp_calib.MD  = combine_bytes(calib_data[20], calib_data[21]);
}

// --- BMP180 Measurement Functions ---

// Read raw temperature from the BMP180
// Sends command 0x2E to register 0xF4, waits 4.5ms, then reads two bytes from 0xF6.
int16_t bmp180_read_raw_temperature() {
    uint8_t command[2] = {0xF4, 0x2E};  // 0x2E: Temperature measurement command
    if (write(bmp_fd, command, 2) != 2) {
        perror("BMP180: Failed to write temperature command");
        exit(1);
    }
    usleep(4500); // Wait 4.5ms for conversion
    
    uint8_t reg = 0xF6;
    uint8_t buffer[2];
    if (write(bmp_fd, &reg, 1) != 1) {
        perror("BMP180: Failed to set temperature data register");
        exit(1);
    }
    if (read(bmp_fd, buffer, 2) != 2) {
        perror("BMP180: Failed to read raw temperature");
        exit(1);
    }
    
    return combine_bytes(buffer[0], buffer[1]);
}

// Read raw pressure from the BMP180
// Sends command 0x34+(OSS<<6) to register 0xF4, waits for conversion,
// then reads 3 bytes from 0xF6 and returns the raw pressure value.
int32_t bmp180_read_raw_pressure() {
    uint8_t command[2] = {0xF4, 0x34 + (OSS << 6)};
    if (write(bmp_fd, command, 2) != 2) {
        perror("BMP180: Failed to write pressure command");
        exit(1);
    }
    // Wait for conversion time depending on OSS
    switch (OSS) {
        case 0: usleep(4500); break;
        case 1: usleep(7500); break;
        case 2: usleep(13500); break;
        case 3: usleep(25500); break;
        default: usleep(4500); break;
    }
    
    uint8_t reg = 0xF6;
    uint8_t buffer[3];
    if (write(bmp_fd, &reg, 1) != 1) {
        perror("BMP180: Failed to set pressure data register");
        exit(1);
    }
    if (read(bmp_fd, buffer, 3) != 3) {
        perror("BMP180: Failed to read raw pressure");
        exit(1);
    }
    
    int32_t raw = ((buffer[0] << 16) | (buffer[1] << 8) | buffer[2]) >> (8 - OSS);
    return raw;
}

// Calculate the true pressure (in hPa) and altitude (in m)
// Temperature is still measured for compensation, but not output.
void parse_bpm180_data(BMP180Data *data) {
    int32_t UT = bmp180_read_raw_temperature();
    int32_t UP = bmp180_read_raw_pressure();
    
    // Calculate true temperature (needed for pressure compensation)
    int32_t X1 = ((UT - bmp_calib.AC6) * bmp_calib.AC5) >> 15;
    int32_t X2 = (bmp_calib.MC << 11) / (X1 + bmp_calib.MD);
    int32_t B5 = X1 + X2;
    // Temperature in °C (not output but needed for compensation)
    // float T = ((B5 + 8) >> 4) / 10.0;
    
    // Calculate true pressure
    int32_t B6 = B5 - 4000;
    X1 = (bmp_calib.B2 * ((B6 * B6) >> 12)) >> 11;
    X2 = (bmp_calib.AC2 * B6) >> 11;
    int32_t X3 = X1 + X2;
    int32_t B3 = ((((int32_t)bmp_calib.AC1) * 4 + X3) << OSS  + 2) >> 2;
    X1 = (bmp_calib.AC3 * B6) >> 13;
    X2 = (bmp_calib.B1 * ((B6 * B6) >> 12)) >> 16;
    X3 = ((X1 + X2) + 2) >> 2;
    uint32_t B4 = (bmp_calib.AC4 * (uint32_t)(X3 + 32768)) >> 15;
    uint32_t B7 = ((uint32_t)UP - B3) * (50000 >> OSS);
    int32_t p;
    if (B7 < 0x80000000) {
        p = (B7 * 2) / B4;
    } else {
        p = (B7 / B4) * 2;
    }
    X1 = (p >> 8) * (p >> 8);
    X1 = (X1 * 3038) >> 16;
    X2 = (-7357 * p) >> 16;
    p = p + ((X1 + X2 + 3791) >> 4);
    
    // Convert pressure from Pa to hPa
    float pressure = p / 100.0;
    
    // Calculate altitude using the barometric formula
    float altitude = 44330.0 * (1.0 - pow(pressure / 1013.25, 0.1903));
    
    data->pressure = pressure;
    data->altitude = altitude;
}

// Print only pressure and altitude data
void print_bmp180_data(const BMP180Data *data) {
    printf("BMP180:\n");
    printf("  Pressure: %f hPa\n", data->pressure);
    printf("  Altitude: %f m\n", data->altitude);
}

// High-level function to perform a complete sensor read and display the results
void spin_bpm() {
    parse_bpm180_data(&bmp_data);
    print_bmp180_data(&bmp_data);
}

// Close the I2C connection for BMP180
void terminate_bmp_connection() {
    close(bmp_fd);
}

// int main() {
    
//     bpm180_init();

//     // Loop continuously reading and printing pressure and altitude data
//     while (1) {
//         spin_bpm();
//         usleep(500000); // Delay for 500 ms
//     }
    
//     terminate_bmp_connection();
//     return 0;
// }
