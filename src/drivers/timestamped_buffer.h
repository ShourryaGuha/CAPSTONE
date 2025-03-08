// shared_buffer.h
#ifndef SHARED_BUFFER_H
#define SHARED_BUFFER_H

#include <pthread.h>
#include <time.h>
#include "imu.h"
#include "gps.h"
#include "barometer.h"
#include "laserTOF.h"

// Unified sensor data structure
typedef struct {
    struct timespec timestamp;
    MPU6050Data imu_data;
    GPSData gps_data;
    BMP180Data baro_data;
    Measurement laser_data;
    int sensors_present;  // Bit flags to indicate which sensors have data
    #define IMU_PRESENT     0x01
    #define GPS_PRESENT     0x02
    #define BARO_PRESENT    0x04
    #define LASER_PRESENT   0x08
} SensorPacket;

// Circular buffer for sensor data
#define BUFFER_SIZE 100

typedef struct {
    SensorPacket buffer[BUFFER_SIZE];
    int write_index;
    int read_index;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t data_available;
} SensorBuffer;

// Initialize the buffer
void buffer_init(SensorBuffer* buffer);

// Add data to the buffer
int buffer_add(SensorBuffer* buffer, SensorPacket* data);

// Get data from the buffer
int buffer_get(SensorBuffer* buffer, SensorPacket* data);

// Clean up the buffer
void buffer_cleanup(SensorBuffer* buffer);

#endif // SHARED_BUFFER_H