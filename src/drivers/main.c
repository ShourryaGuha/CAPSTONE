#include <stdio.h>
#include <unistd.h>
#include "gps.h"
<<<<<<< HEAD
#include "laserTOF.h"
=======
#include "servo_control.h"
>>>>>>> main

int main() {
    // Placeholder for now
    printf("I look forward to the war crimes that will commence shortly\n");

<<<<<<< HEAD
    // gps_init();
    // while(1) {
    //     test_read();
    //     usleep(10000);
    // }
    // terminate_connection();
    // return 0;

    // Laser_Device sensor1, sensor2;
    // float distance;

    // sensor1.device_path = UART3_DEVICE;
    // if (Laser_Init(&sensor1) != 0) {
    //     fprintf(stderr, "Failed to initialize sensor 1\n");
    //     return 1;
    // }
    
    // // Take measurements from both sensors
    // Laser_Measure(&sensor1, MODE_AUTO_1_SHOT, &distance);
    // printf("Sensor 1 distance: %.2f m\n", distance);
    
    // return 0;

    Laser_Device sensor1;
    
    sensor1.device_path = UART3_DEVICE;
    if (Laser_Init(&sensor1) != 0) {
        fprintf(stderr, "Failed to initialize sensor 1\n");
        return 1;
    }

    // Try reading module status
    int status = Laser_ReadStatus(&sensor1);
    if (status < 0) {
        fprintf(stderr, "Failed to read module status\n");
    } else {
        printf("Sensor Status: 0x%04X\n", status);
    }

    return 0;


=======
    // Initialize two servo motors
    servo_t servo1, servo2;
    if (servo_init(&servo1, PWM0_GPIO) < 0) {
        fprintf(stderr, "Failed to initialize servo 1\n");
        return -1;
    }
    if (servo_init(&servo2, PWM1_GPIO) < 0) {
        fprintf(stderr, "Failed to initialize servo 2\n");
        servo_cleanup(&servo1);
        return -1;
    }

    // Move both servos to 25 degrees
    if (servo_set_angle(&servo1, 25.0f) < 0) {
        fprintf(stderr, "Failed to set angle for servo 1\n");
    }
    if (servo_set_angle(&servo2, 25.0f) < 0) {
        fprintf(stderr, "Failed to set angle for servo 2\n");
    }

    // Simulate some delay to observe servo movement
    sleep(2);

    // Cleanup resources
    servo_cleanup(&servo1);
    servo_cleanup(&servo2);

    gps_init();
    while(1) {
        spin_gps();
        usleep(10000);
        test_final_values();
    }
    terminate_connection();
    return 0;
>>>>>>> main
}
