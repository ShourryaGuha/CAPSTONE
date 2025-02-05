#ifndef SOLENOID_CONTROL_H
#define SOLENOID_CONTROL_H

#include <pigpio.h>
#include <stdio.h>

#define SOLENOID_PIN 24  // GPIO 24 (BCM numbering)

// Initializes the solenoid pin
void initializeSolenoid();

// Releases the solenoid (sets GPIO HIGH)
void releaseSolenoid();

// Engages the solenoid (sets GPIO LOW)
void engageSolenoid();

#endif // SOLENOID_CONTROL_H