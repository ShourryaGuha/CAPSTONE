// main.c with simplified buffer
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <unistd.h>
#include <pigpio.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include "gps.h"
#include "barometer.h"
#include "imu.h"
#include "laserTOF.h"
#include "timestamped_buffer.h"

// Global variable for the buffer (in shared memory)
SensorBuffer* g_sensor_buffer;
volatile int running = 1;

// Signal handler for graceful shutdown
void handle_signal(int sig) {
    printf("Shutting down...\n");
    running = 0;
}

// Function to read barometer and update buffer
void read_barometer(SensorBuffer* buffer) {
    SensorData data = {0};
    BMP180Data baro_data;
    
    // Get barometer data
    baro_data = *spin_bpm();  // Function to read barometer data
    
    // Fill the data structure
    clock_gettime(CLOCK_REALTIME, &data.timestamp);
    data.baro.pressure = baro_data.pressure;
    data.baro.altitude = baro_data.altitude;
    data.sensors_present = BARO_PRESENT;
    
    // Add to buffer
    buffer_add(buffer, &data);
}

// Function to read GPS and update buffer
void read_gps(SensorBuffer* buffer) {
    SensorData data = {0};
    GPSData* gps_data;
    
    // Get GPS data
    gps_data = spin_gps();  // Function to read GPS data
    
    // Fill the data structure
    clock_gettime(CLOCK_REALTIME, &data.timestamp);
    
    // Copy GPS data to our structure
    data.gps.latitude = gps_data->latitude;
    data.gps.longitude = gps_data->longitude;
    data.gps.height = gps_data->height;
    data.gps.utc_time = gps_data->utc_time;
    data.gps.velocity_x = gps_data->velocity_x;
    data.gps.velocity_y = gps_data->velocity_y;
    data.gps.velocity_z = gps_data->velocity_z;
    data.gps.accel_x = gps_data->accel_x;
    data.gps.accel_y = gps_data->accel_y;
    data.gps.accel_z = gps_data->accel_z;
    
    data.sensors_present = GPS_PRESENT;
    
    // Add to buffer
    buffer_add(buffer, &data);
}

// Function to read IMU and update buffer
void read_imu(SensorBuffer* buffer) {
    SensorData data = {0};
    MPU6050Data imu_data;
    
    // Get IMU data
    imu_data = *spin_imu();  // Function to read IMU data
    
    // Fill the data structure
    clock_gettime(CLOCK_REALTIME, &data.timestamp);
    
    // Copy IMU data to our structure
    data.imu.ax = imu_data.ax;
    data.imu.ay = imu_data.ay;
    data.imu.az = imu_data.az;
    data.imu.gx = imu_data.gx;
    data.imu.gy = imu_data.gy;
    data.imu.gz = imu_data.gz;
    
    data.sensors_present = IMU_PRESENT;
    
    // Add to buffer
    buffer_add(buffer, &data);
}

// Function to read laser TOF and update buffer
void read_laser(SensorBuffer* buffer, Laser_Device* sensor) {
    SensorData data = {0};
    
    // Get laser data
    if (Laser_Measure_Single(sensor) == 0) {
        // Fill the data structure
        clock_gettime(CLOCK_REALTIME, &data.timestamp);
        data.laser.distance = sensor->distance;
        data.sensors_present = LASER_PRESENT;
        
        // Add to buffer
        buffer_add(buffer, &data);
    }
}

int main() {
    // Set up signal handling for graceful shutdown
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // For nanosleep
    struct timespec ts;
    
    // Initialize shared memory for the buffer
    int fd = shm_open("sensor_buffer", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }
    
    printf("sensor buffer size: %d\n", sizeof(SensorBuffer));
    printf("Successfully created shared memory: /dev/shm/sensor_buffer with fd=%d\n", fd);
    
    if (ftruncate(fd, sizeof(SensorBuffer)) == -1) {
        perror("ftruncate");
        return 1;
    }
    printf("DEBUG: ftruncate success, size set to %zu bytes\n", sizeof(SensorBuffer));
    
    g_sensor_buffer = (SensorBuffer*)mmap(NULL, sizeof(SensorBuffer), 
                                         PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (g_sensor_buffer == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    printf("DEBUG: mmap result: %p, errno=%d (%s)\n", 
           g_sensor_buffer, errno, g_sensor_buffer == MAP_FAILED ? strerror(errno) : "success");
    
    // Initialize the buffer
    buffer_init(g_sensor_buffer);
    
    // Initialize all sensors
    bpm180_init();
    gps_init();
    mpu6050_init();
    
    Laser_Device laser_sensor;
    laser_sensor.device_path = UART3_DEVICE;
    if (Laser_Init(&laser_sensor) != 0) {
        fprintf(stderr, "Failed to initialize laser sensor\n");
        return 1;
    }
    
    // Main sensor reading loop
    while (running) {
        // Read from each sensor and update the buffer
        read_barometer(g_sensor_buffer);
        read_gps(g_sensor_buffer);
        read_imu(g_sensor_buffer);
        read_laser(g_sensor_buffer, &laser_sensor);
        
        // Sleep for 10ms
        ts.tv_sec = 0;
        ts.tv_nsec = 10000 * 1000;  // Convert 10000 microseconds to nanoseconds
        nanosleep(&ts, NULL);
    }
    
    // Clean up
    buffer_cleanup(g_sensor_buffer);
    munmap(g_sensor_buffer, sizeof(SensorBuffer));
    shm_unlink("sensor_buffer");
    
    terminate_bmp_connection();
    terminate_connection();
    terminate_imu_connection();
    Laser_stop_continuous(&laser_sensor);
    
    return 0;
}