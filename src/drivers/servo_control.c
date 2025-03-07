#include <stdio.h>
#include <stdlib.h>
#include <pigpio.h>
#include <unistd.h>
#include "servo_control.h"

// PWM frequency in Hz (50Hz standard for servos)
#define PWM_FREQUENCY 50
// Pulse width limits (in microseconds)
#define MIN_PULSE_WIDTH 500  // 0.5ms pulse width (0 degrees)
#define MAX_PULSE_WIDTH 2500 // 2.5ms pulse width (180 degrees)

// Convert angle to pulse width in microseconds
static int angle_to_pulse_us(float angle) {
    // Constrain angle to valid range
    if (angle < MIN_ANGLE) angle = MIN_ANGLE;
    if (angle > MAX_ANGLE) angle = MAX_ANGLE;
    
    // Map angle (0-180) to pulse width (500-2500 us)
    return MIN_PULSE_WIDTH + (angle / MAX_ANGLE) * (MAX_PULSE_WIDTH - MIN_PULSE_WIDTH);
}

// Initialize a servo motor
int servo_init(servo_t *servo, int gpio_pin) {
    if (servo == NULL) {
        return -1;
    }

    // Check if pigpio is initialized
    static int pigpio_initialized = 0;
    if (!pigpio_initialized) {
        if (gpioInitialise() < 0) {
            fprintf(stderr, "Failed to initialize pigpio\n");
            return -1;
        }
        pigpio_initialized = 1;
    }

    // Verify GPIO pin is valid
    if (gpio_pin != PWM0_GPIO && gpio_pin != PWM1_GPIO) {
        fprintf(stderr, "GPIO pin %d is not configured for servo use\n", gpio_pin);
        return -1;
    }

    // Initialize servo state
    servo->gpio_pin = gpio_pin;
    servo->current_angle = 90.0f;  // Default to middle position
    servo->enabled = 1;

    // Set the GPIO to output mode
    gpioSetMode(gpio_pin, PI_OUTPUT);

    // Set PWM frequency (50Hz is standard for servos)
    gpioSetPWMfrequency(gpio_pin, PWM_FREQUENCY);
    
    // // Set initial position to 90 degrees
    // int pulse_width = angle_to_pulse_us(90.0f);
    // if (gpioServo(gpio_pin, pulse_width) != 0) {
    //     fprintf(stderr, "Failed to set initial servo position on GPIO %d\n", gpio_pin);
    //     return -1;
    // }

    // Allow time for the servo to move to initial position
    usleep(500000);  // 500ms

    return 0;
}

// Set servo angle (0-180 degrees)
int servo_set_angle(servo_t *servo, float angle) {
    if (servo == NULL || !servo->enabled) {
        return -1;
    }

    // Convert angle to pulse width in microseconds
    int pulse_width = angle_to_pulse_us(angle);
    
    // Set servo position
    if (gpioServo(servo->gpio_pin, pulse_width) != 0) {
        fprintf(stderr, "Failed to set servo angle on GPIO %d\n", servo->gpio_pin);
        return -1;
    }

    servo->current_angle = angle;
    return 0;
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

    // Center the servo before cleanup
    //servo_set_angle(servo, 90);
    usleep(500000);  // Wait for movement

    // Stop PWM signal
    gpioServo(servo->gpio_pin, 0);
    
    servo->enabled = 0;
    
    // Note: We don't call gpioTerminate() here as it would affect all GPIO pins
    // The main program should call gpioTerminate() when completely done with all GPIO operations
}