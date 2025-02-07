#include <stdio.h>
#include <unistd.h>
#include "gps.h"
#include "laserTOF.h"

int main() {
    // Placeholder for now
    printf("I look forward to the war crimes that will commence shortly\n");

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


}
