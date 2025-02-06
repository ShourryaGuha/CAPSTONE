#include <stdio.h>
#include <unistd.h>
//#include "gps.h"
#include "imu.c"

int main() {
    mpu6050_init();

    while (1) {
        spin_imu();
        usleep(10000);
    }

    terminate_imu_connection();
    return 0;
}
