#include <stdio.h>
#include <unistd.h>
#include <pigpio.h>
#include "gps.h"
#include "servo_control.h"
#include "barometer.h"
#include "imu.c"
#include "transmitter.h"
#include "laserTOF.h"
#include "solenoid_control.h"

int main() {
    
    //BAROMETER CODE 
    bpm180_init();
    
    // Loop continuously reading and printing pressure and altitude data
    while (1) {
        spin_bpm();
        usleep(500000); // Delay for 500 ms
    terminate_bmp_connection();

    // //GPS CODE
    // gps_init();
    // while(1) {
    //     spin_gps();
    //     usleep(10000);
    //     test_final_values();
    // }

    //IMU CODE
    // mpu6050_init();

    // while (1) {
    //     spin_imu();
    //     usleep(10000);
    // }

    // terminate_imu_connection();

    //LASER TOF CODE
    // Laser_Device sensor1;
    
    // sensor1.device_path = UART2_DEVICE;
    // if (Laser_Init(&sensor1) != 0) {
    //     fprintf(stderr, "Failed to initialize sensor 1\n");
    //     return 1;
    // }
    // // float distance;

    // Laser_Measure_Single(&sensor1);

    // // Initialize the solenoid
    // initializeSolenoid
    // // Release the solenoid for 3 seconds
    
    // while(1){
    //     printf("Releasing solenoid...\n");
    //     releaseSolenoid();
    //     sleep(3);  // Wait for 3 seconds
       
    //     // Re-engage the solenoid
    //     printf("Engaging solenoid...\n");
    //     engageSolenoid();
    //     sleep(3);
    // }

    //TRANSMITTER CODE
    // // Initialize the NRF24L01+ module.
    // nrf24_init();

    // // Allow time for the module to fully power up
    // usleep(200000);  // 200ms delay

    
    // // Prepare our sensor data structure with initial values.
    // SensorData sensorData;
    // sensorData.distance = 100; // Example starting value for distance
    // sensorData.servo1   = 45;  // Example starting angle for servo 1
    // sensorData.servo2   = 90;  // Example starting angle for servo 2
    
    // printf("Starting data transmission. Press Ctrl+C to stop.\n");
    
    // // Main loop: update sensor values (simulated) and transmit them.
    // while (1) {
    //     // (Simulated) Update sensor values. Replace these with actual sensor readings.
    //     sensorData.distance++;         // Increment distance
    //     if (sensorData.distance > 1000)
    //         sensorData.distance = 0;
    //     sensorData.servo1 = (sensorData.servo1 + 1) % 181; // Cycle 0-180
    //     sensorData.servo2 = (sensorData.servo2 + 2) % 181; // Cycle 0-180

    //     // Transmit the sensorData structure as a 6-byte payload.
    //     nrf24_transmit_payload((uint8_t*)&sensorData, sizeof(sensorData));
    // }
    
    // // Cleanup (this code is unreachable in the infinite loop, but shown for completeness).
    // terminate_transmitter();
    
    // return 0;

    //SERVO CODE
    // // Initialize two servo motors
    // servo_t servo1, servo2;
    // if (servo_init(&servo1, PWM0_GPIO) < 0) {
    //     fprintf(stderr, "Failed to initialize servo 1\n");
    //     return -1;
    // }
<<<<<<< HEAD

    // sleep(1);

    // if (servo_init(&servo2, PWM1_GPIO) < 0) {
    //     fprintf(stderr, "Failed to initialize servo 2\n");
    //     return -1;
    // }

    // sleep(1);

    // // Move servo 2 to 90 degrees
    // if (servo_set_angle(&servo2, 80.0f) < 0) {
    //     fprintf(stderr, "Failed to set angle for servo 2\n");
    // }

    // sleep(2);
    
    // // Move servo 2 to 120 degrees
    // if (servo_set_angle(&servo2, 120.0f) < 0) {
=======
    // if (servo_init(&servo2, PWM1_GPIO) < 0) {
    //     fprintf(stderr, "Failed to initialize servo 2\n");
    //     servo_cleanup(&servo1);
    //     return -1;
    // }

    // // Move both servos to 25 degrees
    // if (servo_set_angle(&servo1, 25.0f) < 0) {
    //     fprintf(stderr, "Failed to set angle for servo 1\n");
    // }
    // if (servo_set_angle(&servo2, 25.0f) < 0) {
>>>>>>> solenoid_control
    //     fprintf(stderr, "Failed to set angle for servo 2\n");
    // }

    // // Simulate some delay to observe servo movement
    // sleep(2);

<<<<<<< HEAD
    // // Move servo 1 to 90 degrees
    // if (servo_set_angle(&servo1, 80.0f) < 0) {
    //     fprintf(stderr, "Failed to set angle for servo 1\n");
    // }
    
    // sleep(2);
    
    // // Move servo 1 to 120 degrees
    // if (servo_set_angle(&servo1, 120.0f) < 0) {
    //     fprintf(stderr, "Failed to set angle for servo 1\n");
    // }

    // // Cleanup resources
    // servo_cleanup(&servo1);
    // servo_cleanup(&servo2);
    
    // // Terminate pigpio library
    // gpioTerminate();

    return 0;
}
=======
    // // Cleanup resources
    // servo_cleanup(&servo1);
    // servo_cleanup(&servo2);

    // printf("Servos probably successful\n");

    // gps_init();
    // while(1) {
    //     spin_gps();
    //     usleep(10000);
    //     test_final_values();
    // }
    // terminate_connection();
    // return 0;
}
>>>>>>> solenoid_control
