#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pigpio.h>
#include <unistd.h>

// Pin definitions
#define PIN_PWREN 23  // Power enable pin
#define PIN_TX    14  // UART TX
#define PIN_RX    15  // UART RX

// Constants
#define BAUD_RATE 19200
#define MAX_BUFFER_SIZE 32
#define DEFAULT_SLAVE_ADDR 0x00

// Status codes from manual
typedef enum {
    STATUS_OK = 0x0000,
    STATUS_LOW_POWER = 0x0001,
    STATUS_TEMP_LOW = 0x0003,
    STATUS_TEMP_HIGH = 0x0004,
    STATUS_OUT_OF_RANGE = 0x0005,
    STATUS_INVALID_RESULT = 0x0006,
    STATUS_STRONG_BACKGROUND = 0x0007,
    STATUS_WEAK_SIGNAL = 0x0008,
    STATUS_STRONG_SIGNAL = 0x0009,
    STATUS_UNSTABLE = 0x000F,
    STATUS_INVALID_FRAME = 0x0081
} Laser_Status;

// Measurement modes
typedef enum {
    MODE_AUTO = 0x00,
    MODE_SLOW = 0x01,
    MODE_FAST = 0x02
} Laser_Mode;

typedef struct {
    int handle;         // UART handle
    uint8_t address;    // Device address
} Laser_Device;

// Function prototypes
static uint8_t calculate_checksum(uint8_t *data, int length);
static int validate_response(uint8_t *response, int length);

// Initialize the laser rangefinder
int Laser_Init(Laser_Device *device) {
    // Initialize pigpio
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialization failed.\n");
        exit(EXIT_FAILURE);
        return;
    }

    // Configure pins
    //https://abyz.me.uk/rpi/pigpio/cif.html#gpioSetMode
    gpioSetMode(PIN_PWREN, PI_OUTPUT);
    
    // Power up sequence section 6.2
    gpioWrite(PIN_PWREN, 0);  // Power down first
    usleep(10000);            // Wait 10ms
    gpioWrite(PIN_PWREN, 1);  // Power up
    usleep(100000);           // Wait 100ms for boot
    
    // Open UART
    // serOpen: https://abyz.me.uk/rpi/pigpio/cif.html#serOpen
    // returns a device handle which is just an integer, not sure how that works but hopefully it does
    // this device handle is then used for all other serial operations
    device->handle = serOpen("/dev/ttyAMA0", BAUD_RATE, 0);
    if (device->handle < 0) {
        gpioTerminate();
        fprintf(stderr, "Failed to receive laser uart handle.\n");
        exit(EXIT_FAILURE);
        return;
    }
    
    device->address = DEFAULT_SLAVE_ADDR;
    
    // Auto baud rate detection
    uint8_t autobaud = 0x55;
    serWrite(device->handle, &autobaud, 1);
    usleep(100000); // Wait for reply
    
    uint8_t reply;
    if (serRead(device->handle, &reply, 1) != 1) {
        serClose(device->handle);
        gpioTerminate();
        return -3;
    }
    
    return 0;
}

// Close and cleanup
void Laser_Close(Laser_Device *device) {
    gpioWrite(PIN_PWREN, 0);  // Power down
    serClose(device->handle);
    gpioTerminate();
}

// Perform single measurement
int Laser_Measure(Laser_Device *device, Laser_Mode mode, float *distance) {
    uint8_t cmd[] = {
        0xAA,                   // Header
        0x00,                   // Write command
        device->address,        // Slave address
        0x00, 0x20,            // Register (MEA_START)
        0x00, 0x01,            // Payload count
        0x00, mode,            // Mode
        0x00                    // Checksum (calculated below)
    };
    
    cmd[9] = calculate_checksum(&cmd[2], 7);
    
    // Send command
    serWrite(device->handle, cmd, sizeof(cmd));
    
    // Read response
    uint8_t response[12];
    int bytes_read = 0;
    int timeout = 5000; // 5 second timeout
    
    while (bytes_read < 12 && timeout > 0) {
        int n = serRead(device->handle, &response[bytes_read], 12 - bytes_read);
        if (n > 0) {
            bytes_read += n;
        }
        usleep(1000);
        timeout--;
    }
    
    if (bytes_read != 12) {
        return -1;
    }
    
    // Validate response
    if (!validate_response(response, bytes_read)) {
        return -2;
    }
    
    // Extract distance value (4 bytes starting at index 6)
    uint32_t dist_mm = (response[6] << 24) | (response[7] << 16) | 
                      (response[8] << 8) | response[9];
    *distance = dist_mm / 1000.0f; // Convert to meters
    
    return 0;
}

// Read device status
Laser_Status Laser_GetStatus(Laser_Device *device) {
    uint8_t cmd[] = {
        0xAA,                   // Header
        0x80,                   // Read command
        device->address,        // Slave address
        0x00, 0x00,            // Register (ERR_CODE)
        0x80                    // Checksum
    };
    
    serWrite(device->handle, cmd, sizeof(cmd));
    
    uint8_t response[9];
    int bytes_read = serRead(device->handle, response, sizeof(response));
    
    if (bytes_read != 9) {
        return STATUS_INVALID_FRAME;
    }
    
    return (Laser_Status)((response[6] << 8) | response[7]);
}

// Calculate checksum
static uint8_t calculate_checksum(uint8_t *data, int length) {
    uint8_t sum = 0;
    for (int i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

// Validate response frame
static int validate_response(uint8_t *response, int length) {
    if (length < 3) return 0;
    
    // Check header
    if (response[0] != 0xAA && response[0] != 0xEE) return 0;
    
    // Calculate and verify checksum
    uint8_t calc_checksum = calculate_checksum(&response[1], length - 2);
    return (calc_checksum == response[length - 1]);
}