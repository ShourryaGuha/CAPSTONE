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

    Laser_Device sensor1, sensor2;
    float distance;

    sensor1.device_path = UART2_DEVICE;
    if (Laser_Init(&sensor1) != 0) {
        fprintf(stderr, "Failed to initialize sensor 1\n");
        exit(1);
    }
    
    // Initialize both sensors
    // init_both_sensors(&sensor1, &sensor2);
    
    // Take measurements from both sensors
    Laser_Measure(&sensor1, MODE_AUTO_1_SHOT, &distance);
    printf("Sensor 1 distance: %.2f m\n", distance);
    
    // Laser_Measure(&sensor2, MODE_AUTO_1_SHOT, &distance);
    // printf("Sensor 2 distance: %.2f m\n", distance);
    
    return 0;


}
