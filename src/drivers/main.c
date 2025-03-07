#include <stdio.h>
#include <unistd.h>
#include <pigpio.h>
#include "gps.h"
#include "servo_control.h"

int main() {
    // Placeholder for now
    printf("I look forward to the war crimes that will commence shortly\n");

    // Initialize two servo motors
    servo_t servo1, servo2;
    if (servo_init(&servo1, PWM0_GPIO) < 0) {
        fprintf(stderr, "Failed to initialize servo 1\n");
        return -1;
    }

    sleep(1);

    if (servo_init(&servo2, PWM1_GPIO) < 0) {
        fprintf(stderr, "Failed to initialize servo 2\n");
        return -1;
    }

    sleep(1);

    // Move servo 2 to 90 degrees
    if (servo_set_angle(&servo2, 80.0f) < 0) {
        fprintf(stderr, "Failed to set angle for servo 2\n");
    }

    sleep(2);
    
    // Move servo 2 to 120 degrees
    if (servo_set_angle(&servo2, 120.0f) < 0) {
        fprintf(stderr, "Failed to set angle for servo 2\n");
    }

    // Simulate some delay to observe servo movement
    sleep(2);

    // Move servo 1 to 90 degrees
    if (servo_set_angle(&servo1, 80.0f) < 0) {
        fprintf(stderr, "Failed to set angle for servo 1\n");
    }
    
    sleep(2);
    
    // Move servo 1 to 120 degrees
    if (servo_set_angle(&servo1, 120.0f) < 0) {
        fprintf(stderr, "Failed to set angle for servo 1\n");
    }

    // Simulate some delay to observe servo movement
    sleep(2);

    // Cleanup resources
    servo_cleanup(&servo1);
    servo_cleanup(&servo2);
    
    // Terminate pigpio library
    gpioTerminate();

    return 0;
}