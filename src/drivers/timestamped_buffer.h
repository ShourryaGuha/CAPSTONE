// simple_buffer.h
#ifndef SIMPLE_BUFFER_H
#define SIMPLE_BUFFER_H

#include <pthread.h>
#include <time.h>

// Sensor data structures
typedef struct {
    float ax, ay, az;
    float gx, gy, gz;
} IMUData;

typedef struct {
    double latitude, longitude, height;
    double utc_time;
    double velocity_x, velocity_y, velocity_z;
    double accel_x, accel_y, accel_z;
} GPSData;

typedef struct {
    float pressure;
    float altitude;
} BaroData;

typedef struct {
    float distance;
} LaserData;

// Flags for indicating which sensors are present in a packet
#define IMU_PRESENT    0x01
#define GPS_PRESENT    0x02
#define BARO_PRESENT   0x04
#define LASER_PRESENT  0x08

// Single data packet containing all sensor data and timestamp
typedef struct {
    struct timespec timestamp;
    IMUData imu;
    GPSData gps;
    BaroData baro;
    LaserData laser;
    int sensors_present;  // Bitfield for which sensors are present
} SensorData;

// Buffer size
#define BUFFER_SIZE 100

// Simple circular buffer
typedef struct {
    SensorData buffer[BUFFER_SIZE];
    int write_index;
    int read_index;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t data_available;
} SensorBuffer;

// Buffer operations
void buffer_init(SensorBuffer* buffer);
int buffer_add(SensorBuffer* buffer, SensorData* data);
int buffer_get(SensorBuffer* buffer, SensorData* data);
int buffer_peek(SensorBuffer* buffer, SensorData* data, int offset);
void buffer_cleanup(SensorBuffer* buffer);

#endif // SIMPLE_BUFFER_H