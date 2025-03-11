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

#define TOLERANCE 0.2
#define MAX_ITERATIONS 500
#define SHM_NAME "servo_shared_memory"

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

// New constants for absolute min/max allowed angles
#define ABS_YAW_MIN -80
#define ABS_YAW_MAX 85
#define ABS_PITCH_MIN -85
#define ABS_PITCH_MAX 40

#define SPIRAL_STEP 0.5f
#define SERVO_ROTATION_SPEED 2333
#define MIN_STEP_SIZE 0.1f
#define SPIRAL_EXPANSION 1.02f
#define ARC_STEP 0.05f
#define LPF_ALPHA 0.2f
#define MAX_SERVO_DEVIATION 15.0f  // Max allowable deviation from target

typedef struct {
    float pitch;
    float yaw;
    float distance;
    pthread_mutex_t mutex;
} SharedMemory;

SharedMemory *shared_mem;

float map_angle_to_servo(float angle, float min_angle, float max_angle, float min_servo, float max_servo, float mid_angle, float mid_servo) {
    // Apply absolute limits first
    if (angle < ABS_PITCH_MIN || angle > ABS_PITCH_MAX) {
        angle = fmax(ABS_PITCH_MIN, fmin(ABS_PITCH_MAX, angle));
    }
    if (angle < ABS_YAW_MIN || angle > ABS_YAW_MAX) {
        angle = fmax(ABS_YAW_MIN, fmin(ABS_YAW_MAX, angle));
    }

    // Apply mapping limits
    angle = fmax(min_angle, fmin(max_angle, angle));
    
    return (angle < mid_angle) ? 
        mid_servo + (min_servo - mid_servo) * (angle - mid_angle) / (min_angle - mid_angle) :
        mid_servo + (max_servo - mid_servo) * (angle - mid_angle) / (max_angle - mid_angle);
}

int calculate_servo_delay(float angle_difference) {
    return fabs(angle_difference) * SERVO_ROTATION_SPEED;
}

void send_to_transmitter(float pitch, float yaw, float measured_distance) {
    printf("TX: Pitch: %.2f, Yaw: %.2f, Distance: %.2f\n", pitch, yaw, measured_distance);
}

int check_for_drop_command() {
    // Placeholder: Replace with actual receiver polling
    return 0;
}

int main() {
    if (gpioInitialise() < 0) return -1;

    servo_t servo_pitch, servo_yaw;
    if (servo_init(&servo_pitch, PWM1_GPIO) < 0 || servo_init(&servo_yaw, PWM0_GPIO) < 0) return -1;

    Laser_Device sensor;
    sensor.device_path = UART2_DEVICE;
    if (Laser_Init(&sensor) != 0) return -1;

    initializeSolenoid();

    #ifdef USE_SHARED_MEMORY
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) return 1;
    ftruncate(fd, sizeof(SharedMemory));
    shared_mem = (SharedMemory *)mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_mem == MAP_FAILED) return 1;
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
        target_pitch = -40;
        target_yaw = 0;
        target_distance = 0.3;
        #endif

        float servo_pitch_angle = map_angle_to_servo(target_pitch, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE, PITCH_SERVO_MIN, PITCH_SERVO_MAX, PITCH_MID_ANGLE, PITCH_MID_SERVO);
        float servo_yaw_angle = map_angle_to_servo(target_yaw, YAW_MIN_ANGLE, YAW_MAX_ANGLE, YAW_SERVO_MIN, YAW_SERVO_MAX, YAW_MID_ANGLE, YAW_MID_SERVO);

        servo_set_angle(&servo_pitch, servo_pitch_angle);
        servo_set_angle(&servo_yaw, servo_yaw_angle);

        int max_delay = fmax(calculate_servo_delay(target_pitch - previous_pitch), calculate_servo_delay(target_yaw - previous_yaw));
        usleep(max_delay);

        previous_pitch = target_pitch;
        previous_yaw = target_yaw;

        float measured_distance;
        float step_size = 0.05f;
        float radius = 0.1f;
        float angle_offset = 0.0f;
        int iterations = 0;

        do {
            #ifdef USE_SHARED_MEMORY
            pthread_mutex_lock(&shared_mem->mutex);
            float new_target_pitch = shared_mem->pitch;
            float new_target_yaw = shared_mem->yaw;
            pthread_mutex_unlock(&shared_mem->mutex);

            if (new_target_pitch != target_pitch || new_target_yaw != target_yaw) {
                target_pitch = new_target_pitch;
                target_yaw = new_target_yaw;
                break;
            }
            #endif

            if (check_for_drop_command()) {
                printf("Drop command received!\n");
                releaseSolenoid();
                sleep(1);
                engageSolenoid();
                break;
            }

            Laser_Measure_Single(&sensor);
            measured_distance = (sensor.distance < 0 || sensor.distance > 600) ? 0 : sensor.distance;
            if (fabs(target_distance - measured_distance) < TOLERANCE) break;

            float angle_step = ARC_STEP / (radius + 1e-6);
            angle_offset += angle_step;
            radius += 0.005f;

            float pitch_adjustment = fmax(-MAX_SERVO_DEVIATION, fmin(MAX_SERVO_DEVIATION, cos(angle_offset) * radius));
            float yaw_adjustment = fmax(-MAX_SERVO_DEVIATION, fmin(MAX_SERVO_DEVIATION, sin(angle_offset) * radius));

            servo_pitch_angle = LPF_ALPHA * map_angle_to_servo(
                target_pitch + pitch_adjustment, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE,
                PITCH_SERVO_MIN, PITCH_SERVO_MAX, PITCH_MID_ANGLE, PITCH_MID_SERVO
            ) + (1 - LPF_ALPHA) * servo_pitch_angle;

            servo_yaw_angle = LPF_ALPHA * map_angle_to_servo(
                target_yaw + yaw_adjustment, YAW_MIN_ANGLE, YAW_MAX_ANGLE,
                YAW_SERVO_MIN, YAW_SERVO_MAX, YAW_MID_ANGLE, YAW_MID_SERVO
            ) + (1 - LPF_ALPHA) * servo_yaw_angle;

            servo_set_angle(&servo_pitch, servo_pitch_angle);
            servo_set_angle(&servo_yaw, servo_yaw_angle);
            send_to_transmitter(servo_pitch_angle, servo_yaw_angle, measured_distance);

            usleep(fmax(calculate_servo_delay(pitch_adjustment), calculate_servo_delay(yaw_adjustment)));

            iterations++;
        } while (iterations < MAX_ITERATIONS);

        send_to_transmitter(target_pitch, target_yaw, measured_distance);
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
