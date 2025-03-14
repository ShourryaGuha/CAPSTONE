#include <stdio.h>
#include <unistd.h>
#include <pigpio.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include "gps.h"
#include "barometer.h"
#include "imu.h"
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