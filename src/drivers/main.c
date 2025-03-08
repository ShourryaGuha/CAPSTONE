#define _POSIX_C_SOURCE 199309L

// main.c
#include <stdio.h>
#include <unistd.h>
#include <pigpio.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include "gps.h"
#include "barometer.h"
#include "imu.h"
#include "laserTOF.h"
#include "timestamped_buffer.h"
#include <sys/time.h>
#include <time.h>


// Global variable for the buffer (in shared memory)
SensorBuffer* g_sensor_buffer;
volatile int running = 1;

// Signal handler for graceful shutdown
void handle_signal(int sig) {
    printf("Shutting down...\n");
    running = 0;
}

// Functions to read from each sensor and update the buffer
void read_barometer(SensorBuffer* buffer) {
    SensorPacket packet;
    BMP180Data baro_data;
    
    // Get barometer data
    baro_data = *spin_bpm();  // Function to read barometer data
    
    // Fill the packet
    clock_gettime(CLOCK_REALTIME, &packet.timestamp);
    packet.baro_data = baro_data;
    packet.sensors_present = BARO_PRESENT;
    
    // Add to buffer
    buffer_add(buffer, &packet);
}

void read_gps(SensorBuffer* buffer) {
    SensorPacket packet;
    GPSData gps_data;
    
    // Get GPS data
    gps_data = *spin_gps();  // Function to read GPS data
    
    // Fill the packet
    clock_gettime(CLOCK_REALTIME, &packet.timestamp);
    packet.gps_data = gps_data;
    packet.sensors_present = GPS_PRESENT;
    
    // Add to buffer
    buffer_add(buffer, &packet);
}

void read_imu(SensorBuffer* buffer) {
    SensorPacket packet;
    MPU6050Data imu_data;
    
    // Get IMU data
    imu_data = *spin_imu();  // Function to read IMU data
    
    // Fill the packet
    clock_gettime(CLOCK_REALTIME, &packet.timestamp);
    packet.imu_data = imu_data;
    packet.sensors_present = IMU_PRESENT;
    
    // Add to buffer
    buffer_add(buffer, &packet);
}

void read_laser(SensorBuffer* buffer, Laser_Device* sensor) {
    SensorPacket packet;
    Measurement laser_data;
    
    // Get laser data
    if (Laser_Measure_Single(sensor) == 0) {
        laser_data.distance = sensor->distance;
                
        // Fill the packet
        clock_gettime(CLOCK_REALTIME, &packet.timestamp);
        packet.laser_data = laser_data;
        packet.sensors_present = LASER_PRESENT;
        
        // Add to buffer
        buffer_add(buffer, &packet);
    }
}

int main() {
    // Set up signal handling for graceful shutdown
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // for usleep alternative
    struct timespec ts;
    
    // Initialize shared memory for the buffer
    int fd = shm_open("/sensor_buffer", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }
    
    if (ftruncate(fd, sizeof(SensorBuffer)) == -1) {
        perror("ftruncate");
        return 1;
    }
    
    g_sensor_buffer = (SensorBuffer*)mmap(NULL, sizeof(SensorBuffer), 
                                        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (g_sensor_buffer == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    
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
        
        ts.tv_sec = 0;
        ts.tv_nsec = 10000 * 1000;  // Convert 10000 microseconds to nanoseconds
        nanosleep(&ts, NULL);
    }
    
    // Clean up
    buffer_cleanup(g_sensor_buffer);
    munmap(g_sensor_buffer, sizeof(SensorBuffer));
    shm_unlink("/sensor_buffer");
    
    terminate_bmp_connection();
    terminate_connection();
    terminate_imu_connection();
    Laser_stop_continuous(&laser_sensor);
    
    return 0;
}