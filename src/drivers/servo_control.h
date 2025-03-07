#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#define PWM0_GPIO 18  // GPIO 18 is PWM channel 0
#define PWM1_GPIO 19  // GPIO 19 is PWM channel 1

#define MIN_ANGLE 0
#define MAX_ANGLE 180

// Servo motor structure
typedef struct {
    int gpio_pin;          // GPIO pin number
    float current_angle;   // Current angle of the servo
    int enabled;           // Current state of the servo
} servo_t;

// Initialize a servo motor
int servo_init(servo_t *servo, int gpio_pin);

// Set servo angle (0-180 degrees)
int servo_set_angle(servo_t *servo, float angle);

// Get current servo angle
float servo_get_angle(servo_t *servo);

// Clean up servo resources
void servo_cleanup(servo_t *servo);

#endif // SERVO_CONTROL_H