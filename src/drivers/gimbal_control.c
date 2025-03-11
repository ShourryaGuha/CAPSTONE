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

#define TOLERANCE 0.15
#define MAX_ITERATIONS 500
#define SHM_NAME "servo_shared_memory"

// Define USE_SHARED_MEMORY to use shared memory, otherwise use user-defined variables
// #define USE_SHARED_MEMORY
#define DEBUG

#define YAW_MIN_ANGLE -87
#define YAW_MAX_ANGLE 90
#define YAW_SERVO_MIN 183
#define YAW_SERVO_MAX 3
#define YAW_MID_ANGLE 0
#define YAW_MID_SERVO 93

#define PITCH_MIN_ANGLE -90
#define PITCH_MAX_ANGLE 86
#define PITCH_SERVO_MIN 176
#define PITCH_SERVO_MAX 0
#define PITCH_MID_ANGLE 0
#define PITCH_MID_SERVO 86

#define SPIRAL_STEP 0.5f
#define SERVO_ROTATION_SPEED 2333  // microseconds per degree at 4.8V
#define MIN_STEP_SIZE 0.1f
#define SPIRAL_EXPANSION 1.02f
#define ARC_STEP 0.05f  // Constant arc-length steps for even density
#define LPF_ALPHA 0.2f

typedef struct {
    float pitch;
    float yaw;
    float distance;
    pthread_mutex_t mutex;
} SharedMemory;

SharedMemory *shared_mem;

float map_angle_to_servo(float angle, float min_angle, float max_angle, float min_servo, float max_servo, float mid_angle, float mid_servo) {
    angle = fmax(min_angle, fmin(max_angle, angle));
    if (angle < mid_angle) {
        return mid_servo + (min_servo - mid_servo) * (angle - mid_angle) / (min_angle - mid_angle);
    } else {
        return mid_servo + (max_servo - mid_servo) * (angle - mid_angle) / (max_angle - mid_angle);
    }
}

int calculate_servo_delay(float angle_difference) {
    return fabs(angle_difference) * SERVO_ROTATION_SPEED;
}

void send_to_transmitter(float pitch, float yaw, float measured_distance) {
    printf("TX: Pitch: %.2f, Yaw: %.2f, Distance: %.2f\n", pitch, yaw, measured_distance);
}

void handle_transmitter_signal() {
    // Placeholder function
}

int main() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "Failed to initialize GPIO\n");
        return -1;
    }

    servo_t servo_pitch, servo_yaw;
    if (servo_init(&servo_pitch, PWM0_GPIO) < 0 || servo_init(&servo_yaw, PWM1_GPIO) < 0) {
        fprintf(stderr, "Failed to initialize servos\n");
        return -1;
    }

    Laser_Device sensor;
    sensor.device_path = UART2_DEVICE;
    if (Laser_Init(&sensor) != 0) {
        fprintf(stderr, "Failed to initialize distance sensor\n");
        return -1;
    }

    initializeSolenoid();

#ifdef USE_SHARED_MEMORY
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

    float target_pitch, target_yaw, target_distance;
    float previous_pitch = 0, previous_yaw = 0;

    while (1) {
#ifdef USE_SHARED_MEMORY
        pthread_mutex_lock(&shared_mem->mutex);
        target_pitch = shared_mem->pitch;
        target_yaw = shared_mem->yaw;
        target_distance = shared_mem->distance;
        pthread_mutex_unlock(&shared_mem->mutex);
#else
        target_pitch = 0.648363;
        target_yaw = 1.009765;
        target_distance = 0.3;
#endif

        float servo_pitch_angle = map_angle_to_servo(target_pitch, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE, PITCH_SERVO_MIN, PITCH_SERVO_MAX, PITCH_MID_ANGLE, PITCH_MID_SERVO);
        float servo_yaw_angle = map_angle_to_servo(target_yaw, YAW_MIN_ANGLE, YAW_MAX_ANGLE, YAW_SERVO_MIN, YAW_SERVO_MAX, YAW_MID_ANGLE, YAW_MID_SERVO);

        servo_set_angle(&servo_pitch, servo_pitch_angle);
        servo_set_angle(&servo_yaw, servo_yaw_angle);

        int delay_time_pitch = calculate_servo_delay(target_pitch - previous_pitch);
        int delay_time_yaw = calculate_servo_delay(target_yaw - previous_yaw);
        int max_delay = fmax(delay_time_pitch, delay_time_yaw);
        usleep(max_delay);

        previous_pitch = target_pitch;
        previous_yaw = target_yaw;

        float measured_distance;

        float step_size = 0.05f; // Tiny movements per step
        float radius = 0.1f;  // Smaller starting radius
        float angle_offset = 0.0f;
        float angle_step;
        float alpha = 0.2f;  // Stronger smoothing


        int iterations = 0;

        do {
            Laser_Measure_Single(&sensor);
            measured_distance = sensor.distance;
        
            float error = target_distance - measured_distance;
            if (fabs(error) < TOLERANCE) break;
        
            // Adjust angle step based on radius to keep even point density
            angle_step = step_size / (radius + 1e-6);
            angle_offset += angle_step;
            radius += 0.005f;  // Slower expansion
        
            float pitch_adjustment = cos(angle_offset) * radius;
            float yaw_adjustment = sin(angle_offset) * radius;
        
            // Blend with previous angles for ultra-smooth motion
            servo_pitch_angle = alpha * map_angle_to_servo(
                target_pitch + pitch_adjustment, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE,
                PITCH_SERVO_MIN, PITCH_SERVO_MAX, PITCH_MID_ANGLE, PITCH_MID_SERVO
            ) + (1 - alpha) * servo_pitch_angle;
        
            servo_yaw_angle = alpha * map_angle_to_servo(
                target_yaw + yaw_adjustment, YAW_MIN_ANGLE, YAW_MAX_ANGLE,
                YAW_SERVO_MIN, YAW_SERVO_MAX, YAW_MID_ANGLE, YAW_MID_SERVO
            ) + (1 - alpha) * servo_yaw_angle;
        
            servo_set_angle(&servo_pitch, servo_pitch_angle);
            servo_set_angle(&servo_yaw, servo_yaw_angle);
        
            // Max of pitch and yaw move times, increased by 2.5x for extra slowness
            float move_time = fmax(fabs(pitch_adjustment), fabs(yaw_adjustment)) * SERVO_ROTATION_SPEED * 2.5;
            usleep((int) move_time);
        
            iterations++;
        } while (fabs(target_distance - measured_distance) > TOLERANCE && iterations < MAX_ITERATIONS);

        send_to_transmitter(target_pitch, target_yaw, measured_distance);
        handle_transmitter_signal();
        usleep(100000);
    }

    servo_cleanup(&servo_pitch);
    servo_cleanup(&servo_yaw);
    gpioTerminate();
#ifdef USE_SHARED_MEMORY
    munmap(shared_mem, sizeof(SharedMemory));
    shm_unlink(SHM_NAME);
#endif
    return 0;
}
