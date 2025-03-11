#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pigpio.h>
#include <math.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include "gps.h"
#include "servo_control.h"
#include "laserTOF.h"
#include "solenoid_control.h"

#define TOLERANCE 0.15 // Midpoint of 0.1 to 0.2m
#define MAX_ITERATIONS 10
#define SHM_NAME "servo_shared_memory"

// Define USE_SHARED_MEMORY to use shared memory, otherwise use user-defined variables
#define USE_SHARED_MEMORY

// Servo calibration values
#define YAW_MIN_ANGLE -90
#define YAW_MAX_ANGLE 90
#define YAW_SERVO_MIN 165
#define YAW_SERVO_MAX 75

#define PITCH_MIN_ANGLE -90
#define PITCH_MAX_ANGLE 45
#define PITCH_SERVO_MIN 180
#define PITCH_SERVO_MAX 60

typedef struct {
    float pitch;
    float yaw;
    float distance;
    pthread_mutex_t mutex;
} SharedMemory;

SharedMemory *shared_mem;

float map_angle_to_servo(float angle, float min_angle, float max_angle, float servo_min, float servo_max) {
    float servo_angle = servo_min + (servo_max - servo_min) * (angle - min_angle) / (max_angle - min_angle);
    return fmax(servo_min, fmin(servo_max, servo_angle)); // Clamping to ensure bounds
}

void send_to_transmitter(float pitch, float yaw, float measured_distance) {
    // Placeholder function for transmitting data
    printf("TX: Pitch: %.2f, Yaw: %.2f, Distance: %.2f\n", pitch, yaw, measured_distance);
}

int receive_from_transmitter() {
    // Placeholder function for receiving data
    printf("RX: Receiving updated values...\n");
    return 0; // Dummy return value for now
}

void handle_transmitter_signal() {
    int signal = receive_from_transmitter();
    if (signal == 1) {
        printf("Releasing solenoid...\n");
        releaseSolenoid();
        sleep(1);  // Wait for 1 second
        printf("Engaging solenoid...\n");
        engageSolenoid();
    }
}

int main() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "Failed to initialize GPIO\n");
        return -1;
    }

    // Initialize servos
    servo_t servo_pitch, servo_yaw;
    if (servo_init(&servo_pitch, PWM0_GPIO) < 0 || servo_init(&servo_yaw, PWM1_GPIO) < 0) {
        fprintf(stderr, "Failed to initialize servos\n");
        return -1;
    }

    // Initialize distance sensor
    Laser_Device sensor;
    sensor.device_path = UART2_DEVICE;
    if (Laser_Init(&sensor) != 0) {
        fprintf(stderr, "Failed to initialize distance sensor\n");
        return -1;
    }

    // Initialize solenoid
    initializeSolenoid();

#ifdef USE_SHARED_MEMORY
    // Attach to shared memory
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }
    ftruncate(fd, sizeof(SharedMemory));
    shared_mem = (SharedMemory *)mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
#endif

    float target_pitch;
    float target_yaw;
    float target_distance;

    while (1) {
#ifdef USE_SHARED_MEMORY
        pthread_mutex_lock(&shared_mem->mutex);
        target_pitch = shared_mem->pitch;
        target_yaw = shared_mem->yaw;
        target_distance = shared_mem->distance;
        pthread_mutex_unlock(&shared_mem->mutex);
#else
        // User-defined values (replace with actual input as needed)
        target_pitch = 45.0; // Example static values
        target_yaw = 30.0;
        target_distance = 5.0;
#endif

        // Map angles to servo values
        float servo_pitch_angle = map_angle_to_servo(target_pitch, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE, PITCH_SERVO_MIN, PITCH_SERVO_MAX);
        float servo_yaw_angle = map_angle_to_servo(target_yaw, YAW_MIN_ANGLE, YAW_MAX_ANGLE, YAW_SERVO_MIN, YAW_SERVO_MAX);

        servo_set_angle(&servo_pitch, servo_pitch_angle);
        servo_set_angle(&servo_yaw, servo_yaw_angle);
        usleep(500000); // Allow servos to move

        float measured_distance;
        int iterations = 0;

        do {
            Laser_Measure_Single(&sensor);
            measured_distance = sensor.distance;
            float error = target_distance - measured_distance;

            if (fabs(error) < TOLERANCE) {
                break;
            }

            // Gradient-based adjustment
            float correction_factor = 0.1f; // Adjust as needed
            float pitch_adjustment = correction_factor * error;
            float yaw_adjustment = correction_factor * error;

            servo_set_angle(&servo_pitch, fmax(PITCH_SERVO_MIN, fmin(PITCH_SERVO_MAX, servo_pitch_angle + pitch_adjustment)));
            servo_set_angle(&servo_yaw, fmax(YAW_SERVO_MIN, fmin(YAW_SERVO_MAX, servo_yaw_angle + yaw_adjustment)));

            usleep(500000);
            iterations++;
        } while (fabs(target_distance - measured_distance) > TOLERANCE && iterations < MAX_ITERATIONS);

        send_to_transmitter(target_pitch, target_yaw, measured_distance);
        handle_transmitter_signal();
        usleep(100000); // Small delay before next loop iteration
    }

    // Cleanup
    servo_cleanup(&servo_pitch);
    servo_cleanup(&servo_yaw);
    gpioTerminate();
#ifdef USE_SHARED_MEMORY
    munmap(shared_mem, sizeof(SharedMemory));
    shm_unlink(SHM_NAME);
#endif
    return 0;
}
