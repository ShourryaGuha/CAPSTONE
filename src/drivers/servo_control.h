#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

// Constants for PWM configuration
#define PWM_CHIP_PATH "/sys/class/pwm/pwmchip0"
#define PERIOD_NS 20000000   // 20ms period (50Hz)
#define MIN_PULSE_NS 500000  // 0.5ms pulse width (0 degrees)
#define MAX_PULSE_NS 2500000 // 2.5ms pulse width (180 degrees)
#define MIN_ANGLE 0
#define MAX_ANGLE 180

// GPIO pins corresponding to PWM channels
#define PWM0_GPIO 18  // GPIO 18 is PWM channel 0
#define PWM1_GPIO 19  // GPIO 19 is PWM channel 1

// Servo structure definition
typedef struct {
    int gpio_pin;          // GPIO pin number
    int pwm_channel;       // PWM channel number
    float current_angle;   // Current angle of the servo
    int enabled;           // Current state of the PWM channel
} servo_t;

/**
 * @brief Initialize a servo motor.
 *
 * @param servo Pointer to a servo_t structure.
 * @param gpio_pin GPIO pin connected to the servo motor.
 * @return 0 on success, -1 on failure.
 */
int servo_init(servo_t *servo, int gpio_pin);

/**
 * @brief Set the angle of the servo motor.
 *
 * @param servo Pointer to a servo_t structure.
 * @param angle Desired angle in degrees (0-180).
 * @return 0 on success, -1 on failure.
 */
int servo_set_angle(servo_t *servo, float angle);

/**
 * @brief Get the current angle of the servo motor.
 *
 * @param servo Pointer to a servo_t structure.
 * @return The current angle of the servo motor, or -1 on error.
 */
float servo_get_angle(servo_t *servo);

/**
 * @brief Clean up resources used by the servo motor.
 *
 * @param servo Pointer to a servo_t structure.
 */
void servo_cleanup(servo_t *servo);

#endif // SERVO_CONTROL_H