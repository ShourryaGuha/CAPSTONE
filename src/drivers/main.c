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

    return 0;
}
