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

    LRF_Device device;
    float distance;

    if (Laser_Init(&device) != 0) {
        fprintf(stderr, "Failed to initialize device\n");
        return 1;
    }

    if (Laser_Measure(&device, MODE_AUTO, &distance) == 0) {
        printf("Distance: %.3f m\n", distance);
    }


}
