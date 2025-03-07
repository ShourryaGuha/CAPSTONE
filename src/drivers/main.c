#include <stdio.h>
#include <unistd.h>
#include "gps.h"
#include "servo_control.h"
#include "barometer.h"
#include "imu.c"

int main() {
    bpm180_init();

    //BAROMETER CODE
    // Loop continuously reading and printing pressure and altitude data
    while (1) {
        spin_bpm();
        usleep(500000); // Delay for 500 ms
    }
    
    terminate_bmp_connection();

    //IMU CODE
    // mpu6050_init();

    // while (1) {
    //     spin_imu();
    //     usleep(10000);
    // }

    // terminate_imu_connection();


