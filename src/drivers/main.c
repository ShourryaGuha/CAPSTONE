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


// #define DEBUG

#define BUFFER_SIZE 100

typedef struct{
    BMP180Data baro_data;
    GPSData gps_data;
    MPU6050Data imu_data;
    float distance;
    double delta_t;
    pthread_mutex_t mutex;
}SensorSuite;

//SHARED MEMORY------------------------------------------------
// Define a simple shared memory structure with just a counter
typedef struct {
    int counter;
    pthread_mutex_t mutex;
} SharedCounter;

// Global variable for shared memory
SensorSuite* g_shared_buffer;
volatile int running = 1;

// Signal handler for graceful shutdown
void handle_signal(int sig) {
    printf("C program: Shutting down...\n");
    running = 0;
}

#ifdef DEBUG
void print_sensor_suite(const SensorSuite* s) {
    printf("===== SensorSuite Data =====\n");
    
    // Print Barometer Data
    printf("Barometer:\n");
    printf("  Pressure: %.2f hPa\n", s->baro_data.pressure);
    printf("  Altitude: %.2f m\n", s->baro_data.altitude);

    // Print GPS Data
    printf("GPS:\n");
    printf("  Latitude: %.6f\n", s->gps_data.latitude);
    printf("  Longitude: %.6f\n", s->gps_data.longitude);
    printf("  Height: %.2f m\n", s->gps_data.height);
    printf("  UTC Time: %.3f s\n", s->gps_data.utc_time);
    printf("  Velocity (X, Y, Z): [%.3f, %.3f, %.3f] m/s\n", 
           s->gps_data.velocity_x, s->gps_data.velocity_y, s->gps_data.velocity_z);
    printf("  Acceleration (X, Y, Z): [%.3f, %.3f, %.3f] m/s²\n", 
           s->gps_data.accel_x, s->gps_data.accel_y, s->gps_data.accel_z);

    // Print IMU Data
    printf("IMU:\n");
    printf("  Acceleration (X, Y, Z): [%.3f, %.3f, %.3f] m/s²\n", 
           s->imu_data.ax, s->imu_data.ay, s->imu_data.az);
    printf("  Gyroscope (X, Y, Z): [%.3f, %.3f, %.3f] rad/s\n", 
           s->imu_data.gx, s->imu_data.gy, s->imu_data.gz);

    // Print Laser TOF Data
    printf("Laser TOF:\n");
    printf("  Distance: %.2f meters\n", s->distance);

    // Print Time Delta
    printf("Time:\n");
    printf("  Delta T: %.6f seconds\n", s->delta_t);

    printf("============================\n");
}
#endif


int main() {
    // clock_t start = clock();

    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);

    // Set up signal handling for graceful shutdown
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // For nanosleep
    struct timespec ts;
    
    // Initialize shared memory for the counter
    int fd = shm_open("simple_counter", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }
    
    printf("Successfully created shared memory: /dev/shm/simple_counter with fd=%d\n", fd);
    
    if (ftruncate(fd, sizeof(SensorSuite)*BUFFER_SIZE) == -1) {
        perror("ftruncate");
        close(fd);
        return 1;
    }
    printf("Size set to %zu bytes\n", sizeof(SensorSuite)*BUFFER_SIZE);
    
    g_shared_buffer = (SensorSuite*)mmap(NULL, sizeof(SensorSuite)*BUFFER_SIZE, 
                                         PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (g_shared_buffer == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    printf("mmap result: %p\n", g_shared_buffer);
    
    // Initialize the counter and mutex
    memset(g_shared_buffer, 0, sizeof(SensorSuite)*BUFFER_SIZE);  // Set all bytes to 0
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&g_shared_buffer->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    
    printf("C program: Starting counter updates\n");
    
    BMP180Data* baro_data;
    GPSData* gps_data;
    MPU6050Data* imu_data;
    
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
    // Laser_Measure_Continuous(&laser_sensor);

    // Main loop - increment the counter
    int i = 0;
    SensorSuite* start_pos = g_shared_buffer;
    while (running) {
        // Lock mutex, update counter, unlock mutex
        pthread_mutex_lock(&g_shared_buffer->mutex);
        
        // copy data to memory
        // g_shared_buffer->counter++;
        gps_data = spin_gps();
        (g_shared_buffer+i)->gps_data = *gps_data;
        
        imu_data = spin_imu();
        (g_shared_buffer+i)->imu_data = *imu_data;
        
        baro_data = spin_bpm();
        (g_shared_buffer+i)->baro_data = *baro_data;

        if (Laser_Measure_Single(&laser_sensor) == 0) {
            (g_shared_buffer+i)->distance = laser_sensor.distance;
        }

        // (g_shared_buffer+i)->delta_t = (double)(clock() - start)/CLOCKS_PER_SEC;
        // (g_shared_buffer+i)->delta_t = difftime(now, start);

        gettimeofday(&current_time, NULL);        
        (g_shared_buffer+i)->delta_t = (double)((current_time.tv_sec - start_time.tv_sec) + (current_time.tv_usec - start_time.tv_usec) / 1000000.0);
        
        pthread_mutex_unlock(&g_shared_buffer->mutex);
        
        //i = (i+1)%BUFFER_SIZE;

        #ifdef DEBUG
            print_sensor_suite(&g_shared_buffer[i]);
        #endif

        // usleep(1);
    }
    
    // Clean up
    pthread_mutex_destroy(&g_shared_buffer->mutex);
    munmap(g_shared_buffer, sizeof(SharedCounter));
    close(fd);
    shm_unlink("simple_counter");
    
    printf("C program: Cleanup complete\n");
    return 0;
}