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
#include "transceiver.h"

int main() {
    
    // Initialize the solenoid
    initializeSolenoid();
    // Release the solenoid for 3 seconds

    printf("Releasing solenoid...\n");
    releaseSolenoid();
    sleep(3);  // Wait for 3 seconds
   
    // Re-engage the solenoid
    printf("Engaging solenoid...\n");
    engageSolenoid();
    sleep(3);
    
}
