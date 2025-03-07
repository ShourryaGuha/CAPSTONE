#include <pigpio.h>
#include <stdio.h>

#define SOLENOID_PIN 24  // GPIO 24 (BCM numbering)

void releaseSolenoid() {
    gpioWrite(SOLENOID_PIN, 1);  // Set GPIO HIGH
}

void engageSolenoid() {
    gpioWrite(SOLENOID_PIN, 0);  // Set GPIO LOW
}

void initializeSolenoid() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "Failed to initialize pigpio\n");
        return;
    }
    gpioSetMode(SOLENOID_PIN, PI_OUTPUT);  // Set the pin as output
    engageSolenoid();  // Default to engaged state
}