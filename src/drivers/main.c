#include <stdio.h>
#include <unistd.h>
// #include "gps.h"
// #include "servo_control.h"
#include "transmitter.h" 

int main() {
  // Initialize the NRF24L01+ module.
    nrf24_init();
    
    // Prepare our sensor data structure with initial values.
    SensorData sensorData;
    sensorData.distance = 100; // Example starting value for distance
    sensorData.servo1   = 45;  // Example starting angle for servo 1
    sensorData.servo2   = 90;  // Example starting angle for servo 2
    
    printf("Starting data transmission. Press Ctrl+C to stop.\n");
    
    // Main loop: update sensor values (simulated) and transmit them.
    while (1) {
        // (Simulated) Update sensor values. Replace these with actual sensor readings.
        sensorData.distance++;         // Increment distance
        if (sensorData.distance > 1000)
            sensorData.distance = 0;
        sensorData.servo1 = (sensorData.servo1 + 1) % 181; // Cycle 0-180
        sensorData.servo2 = (sensorData.servo2 + 2) % 181; // Cycle 0-180

        // Transmit the sensorData structure as a 6-byte payload.
        nrf24_transmit_payload((uint8_t*)&sensorData, sizeof(sensorData));
        
        delay(1);  // A short delay (1 ms) before sending the next payload.
    }
    
    return 0;
}
