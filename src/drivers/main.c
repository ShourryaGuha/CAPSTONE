#include <stdio.h>
#include <unistd.h>
#include "gps.h"
#include "servo_control.h"
#include "barometer.h"

int main() {
    bpm180_init();

    // Loop continuously reading and printing pressure and altitude data
    while (1) {
        spin_bpm();
        usleep(500000); // Delay for 500 ms
    }
    
    terminate_bmp_connection();
    return 0;
}
