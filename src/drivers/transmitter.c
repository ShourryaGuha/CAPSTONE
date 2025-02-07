#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <wiringPi.h>      // For GPIO control
#include <wiringPiSPI.h>   // For SPI communication

// Define the pins and SPI channel/speed
#define CE_PIN      8         // Chip Enable pin (GPIO 8)
#define CSN_PIN     26        // Chip Select Not pin (GPIO 26)
#define SPI_CHANNEL 0         // SPI channel 0 (/dev/spidev0.0)
#define SPI_SPEED   500000    // SPI clock speed (500 kHz)

// Define basic NRF24L01+ commands
#define NRF_W_REGISTER    0x20  // Command for writing to a register
#define NRF_W_TX_PAYLOAD  0xA0  // Command for writing TX payload
#define NRF_FLUSH_TX      0xE1  // Command to flush TX FIFO

// Define a packed structure for our 3 data values (6 bytes total)
typedef struct __attribute__((packed)) {
    uint16_t distance;   // Distance sensor reading
    uint16_t servo1;     // Servo 1 angle
    uint16_t servo2;     // Servo 2 angle
} SensorData;

/*
 * Write a single byte 'value' to the NRF24L01 register 'reg'.
 */
void nrf24_write_register(uint8_t reg, uint8_t value) {
    uint8_t data[2];
    data[0] = NRF_W_REGISTER | (reg & 0x1F);  // Create command byte (register write)
    data[1] = value;                          // Data byte to write
    digitalWrite(CSN_PIN, LOW);               // Begin SPI transaction (CSN low)
    wiringPiSPIDataRW(SPI_CHANNEL, data, 2);  // Send 2 bytes over SPI
    digitalWrite(CSN_PIN, HIGH);              // End SPI transaction (CSN high)
}

/*
 * Write multiple bytes starting at register 'reg'.
 * This is used to set the TX address.
 */
void nrf24_write_register_multi(uint8_t reg, uint8_t *data, uint8_t len) {
    uint8_t buffer[len+1];
    buffer[0] = NRF_W_REGISTER | (reg & 0x1F);  // Command byte
    for (int i = 0; i < len; i++) {
        buffer[i+1] = data[i];                  // Copy data bytes
    }
    digitalWrite(CSN_PIN, LOW);
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, len+1);
    digitalWrite(CSN_PIN, HIGH);
}

/*
 * Flush the TX FIFO to remove any stale data.
 */
void nrf24_flush_tx() {
    uint8_t cmd = NRF_FLUSH_TX;
    digitalWrite(CSN_PIN, LOW);
    wiringPiSPIDataRW(SPI_CHANNEL, &cmd, 1);
    digitalWrite(CSN_PIN, HIGH);
}

/*
 * Transmit a payload of 'len' bytes.
 * This function sends a command byte followed by the payload.
 */
void nrf24_transmit_payload(uint8_t *payload, uint8_t len) {
    uint8_t buffer[len+1];
    buffer[0] = NRF_W_TX_PAYLOAD;  // Command to write TX payload
    for (int i = 0; i < len; i++) {
        buffer[i+1] = payload[i];  // Copy payload data
    }
    digitalWrite(CSN_PIN, LOW);
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, len+1);
    digitalWrite(CSN_PIN, HIGH);

    // Pulse the CE pin high for at least 10 µs to start transmission.
    digitalWrite(CE_PIN, HIGH);
    delayMicroseconds(15);  // 15 µs pulse (safe margin)
    digitalWrite(CE_PIN, LOW);
}

/*
 * Initialize the NRF24L01+ module in TX mode.
 * Only the essential settings are configured.
 */
void nrf24_init() {
    // Initialize WiringPi with BCM (GPIO) pin numbering.
    if (wiringPiSetupGpio() == -1) {
        fprintf(stderr, "WiringPi setup failed\n");
        exit(1);
    }
    
    // Set the CE and CSN pins as outputs.
    pinMode(CE_PIN, OUTPUT);
    pinMode(CSN_PIN, OUTPUT);
    digitalWrite(CSN_PIN, HIGH);  // Ensure CSN is high when idle.
    
    // Initialize SPI on the designated channel and speed.
    if (wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1) {
        fprintf(stderr, "SPI setup failed\n");
        exit(1);
    }

    // Set CONFIG register: power up, enable CRC (2 bytes), and set as transmitter.
    // Bit 3: EN_CRC, Bit 2: CRCO, Bit 1: PWR_UP, Bit 0: PRIM_RX (0 = TX mode).
    nrf24_write_register(0x00, (1<<3) | (1<<2) | (1<<1));
    
    // Set TX address and RX address (pipe 0) to the same 5-byte value.
    uint8_t address[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    nrf24_write_register_multi(0x10, address, 5); // TX_ADDR register (0x10)
    nrf24_write_register_multi(0x0A, address, 5); // RX_ADDR_P0 register (0x0A)
    
    // Flush any residual data in the TX FIFO.
    nrf24_flush_tx();
    
    // Ensure CE is low to keep the module in standby until transmitting.
    digitalWrite(CE_PIN, LOW);
}

int main(void) {

    
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
