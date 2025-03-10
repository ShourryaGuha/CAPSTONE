#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Create an RF24 radio object using Arduino pins 8 (CE) and 10 (CSN)
RF24 radio(8, 10);

// Define the same 5-byte address as used in the transmitter.
// You can use a 64-bit integer literal; note that only the lower 5 bytes are used.
const uint64_t pipe = 0xE7E7E7E7E7LL;

// Define a structure that exactly matches the transmitted data.
// (Three float values: distance, servo1, servo2)
struct SensorData {
  float distance;
  float servo1;
  float servo2;
};

void setup() {
  // Initialize Serial communication for output.
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for serial port to be available (needed for some boards)
  }
  Serial.println("NRF24L01+ Receiver Starting...");

  // Initialize the RF24 radio.
  if (!radio.begin()) {
    Serial.println("Radio hardware not responding!");
    while (!radio.begin()) {
      // Stay here if radio is not responding.
    }
  }

  // Set the same channel as the transmitter (channel 76)
  radio.setChannel(76);

  // Set the data rate to 250kbps to match the transmitter.
  radio.setDataRate(RF24_250KBPS);

  // Set the PA level to high (matching the transmitter's configuration).
  radio.setPALevel(RF24_PA_HIGH);

  // Set the CRC length to 16 bits.
  radio.setCRCLength(RF24_CRC_16);

  // Open reading pipe 0 with the same address.
  radio.openReadingPipe(0, pipe);

  // Start the radio listening (RX mode)
  radio.startListening();

  // (Optional) Print radio configuration for debugging.
  radio.printDetails();
}

void loop() {
  // // Check if there is incoming data
  if (radio.available()) {
    uint8_t rawData[32]; // NRF24 can handle up to 32 bytes per packet
    size_t dataSize = radio.getDynamicPayloadSize(); // Get actual payload size

    if (dataSize > 0 && dataSize <= 32) {
      radio.read(rawData, dataSize);
      
      // Check if the received data size is exactly 12 bytes (three floats)
      if (dataSize == sizeof(SensorData)) {
        SensorData sensorData;
        memcpy(&sensorData, rawData, sizeof(SensorData));

        Serial.print("Distance: ");
        Serial.println(sensorData.distance);
        Serial.print("Servo 1: ");
        Serial.println(sensorData.servo1);
        Serial.print("Servo 2: ");
        Serial.println(sensorData.servo2);
      } else {
        // If data size is unexpected, print raw data in hexadecimal format.
        Serial.print("Raw Data: ");
        for (size_t i = 0; i < dataSize; i++) {
          Serial.print(rawData[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
      }

    }
  }
  
}
