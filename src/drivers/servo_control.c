#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

#define PWM_CHIP_PATH "/sys/class/pwm/pwmchip0"
#define PERIOD_NS 20000000   // 20ms period (50Hz)
#define MIN_PULSE_NS 500000  // 0.5ms pulse width (0 degrees)
#define MAX_PULSE_NS 2500000 // 2.5ms pulse width (180 degrees)
#define MIN_ANGLE 0
#define MAX_ANGLE 180

// PWM channel mapping
#define PWM0_GPIO 18  // GPIO 18 is PWM channel 0
#define PWM1_GPIO 19  // GPIO 19 is PWM channel 1

typedef struct {
    int gpio_pin;          // GPIO pin number
    int pwm_channel;       // PWM channel number
    float current_angle;   // Current angle of the servo
    int enabled;          // Current state of the PWM channel
} servo_t;

// Convert GPIO pin to PWM channel
static int gpio_to_pwm_channel(int gpio_pin) {
    switch (gpio_pin) {
        case PWM0_GPIO:
            return 0;
        case PWM1_GPIO:
            return 1;
        default:
            return -1;
    }
}

// Write value to sysfs file
static int write_to_file(const char *filename, const char *format, ...) {
    char buffer[64];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    int fd = open(filename, O_WRONLY);
    if (fd < 0) {
        return -1;
    }

    int ret = write(fd, buffer, strlen(buffer));
    close(fd);
    return (ret < 0) ? -1 : 0;
}

// Export PWM channel if not already exported
static int export_pwm(int channel) {
    char path[128];
    snprintf(path, sizeof(path), PWM_CHIP_PATH "/pwm%d", channel);
    
    // Check if already exported
    if (access(path, F_OK) == 0) {
        return 0;
    }

    return write_to_file(PWM_CHIP_PATH "/export", "%d", channel);
}

// Convert angle to pulse width in nanoseconds
static long angle_to_pulse_ns(float angle) {
    // Constrain angle to valid range
    if (angle < MIN_ANGLE) angle = MIN_ANGLE;
    if (angle > MAX_ANGLE) angle = MAX_ANGLE;
    
    // Map angle (0-180) to pulse width (500000-2500000 ns)
    return MIN_PULSE_NS + (angle / MAX_ANGLE) * (MAX_PULSE_NS - MIN_PULSE_NS);
}

// Set servo angle (0-180 degrees)
int servo_set_angle(servo_t *servo, float angle) {
    if (servo == NULL || !servo->enabled) {
        return -1;
    }

    char path[128];
    snprintf(path, sizeof(path), PWM_CHIP_PATH "/pwm%d/duty_cycle", 
             servo->pwm_channel);

    long pulse_ns = angle_to_pulse_ns(angle);
    if (write_to_file(path, "%ld", pulse_ns) < 0) {
        fprintf(stderr, "Failed to set duty cycle: %s\n", strerror(errno));
        return -1;
    }

    servo->current_angle = angle;
    return 0;
}

// Initialize a servo motor
int servo_init(servo_t *servo, int gpio_pin) {
    if (servo == NULL) {
        return -1;
    }

    // Convert GPIO pin to PWM channel
    int pwm_channel = gpio_to_pwm_channel(gpio_pin);
    if (pwm_channel < 0) {
        fprintf(stderr, "GPIO pin %d does not support hardware PWM\n", gpio_pin);
        return -1;
    }

    // Export PWM channel
    if (export_pwm(pwm_channel) < 0) {
        fprintf(stderr, "Failed to export PWM for GPIO %d (channel %d): %s\n", 
                gpio_pin, pwm_channel, strerror(errno));
        return -1;
    }

    // Allow time for sysfs to create files
    usleep(100000);

    char path[128];
    
    // Set period (20ms = 20000000 ns)
    snprintf(path, sizeof(path), PWM_CHIP_PATH "/pwm%d/period", pwm_channel);
    if (write_to_file(path, "%d", PERIOD_NS) < 0) {
        fprintf(stderr, "Failed to set period: %s\n", strerror(errno));
        return -1;
    }

    // Initialize servo state
    servo->gpio_pin = gpio_pin;
    servo->pwm_channel = pwm_channel;
    servo->current_angle = 90.0f;  // Default to middle position
    servo->enabled = 0;

    // Enable PWM
    snprintf(path, sizeof(path), PWM_CHIP_PATH "/pwm%d/enable", pwm_channel);
    if (write_to_file(path, "1") < 0) {
        fprintf(stderr, "Failed to enable PWM: %s\n", strerror(errno));
        return -1;
    }
    servo->enabled = 1;

    // Set initial position to 90 degrees
    return servo_set_angle(servo, 90);
}

// Get current servo angle
float servo_get_angle(servo_t *servo) {
    if (servo == NULL) {
        return -1;
    }
    return servo->current_angle;
}

// Clean up servo resources
void servo_cleanup(servo_t *servo) {
    if (servo == NULL || !servo->enabled) {
        return;
    }

    char path[128];

    // Center the servo before cleanup
    servo_set_angle(servo, 90);
    usleep(500000);  // Wait for movement

    // Disable PWM
    snprintf(path, sizeof(path), PWM_CHIP_PATH "/pwm%d/enable", servo->pwm_channel);
    write_to_file(path, "0");

    // Unexport PWM channel
    write_to_file(PWM_CHIP_PATH "/unexport", "%d", servo->pwm_channel);

    servo->enabled = 0;
}