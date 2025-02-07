#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>

// Constants
#define UART2_DEVICE "/dev/ttyAMA2"
#define UART3_DEVICE "/dev/ttyAMA3"
#define BAUD_RATE B19200 // deafault baud rate for laser sensor (avoid autodetect)
#define MAX_BUFFER_SIZE 32
#define DEFAULT_SLAVE_ADDR 0x00
#define INIT_DELAY_US 2500000 // 2.5 seconds in microseconds
#define MAX_MEASUREMENTS 100  // Circular buffer size for measurements

// Status codes from manual
typedef enum
{
    STATUS_OK = 0x0000,
    STATUS_LOW_POWER = 0x0001,
    STATUS_TEMP_LOW = 0x0003,
    STATUS_TEMP_HIGH = 0x0004,
    STATUS_OUT_OF_RANGE = 0x0005,
    STATUS_INVALID_RESULT = 0x0006,
    STATUS_STRONG_BACKGROUND_LIGHT = 0x0007,
    STATUS_WEAK_SIGNAL = 0x0008,
    STATUS_STRONG_SIGNAL = 0x0009,
    STATUS_UNSTABLE = 0x000F,
    STATUS_INVALID_FRAME = 0x0081
} Laser_Status;

// Measurement modes
typedef enum
{
    MODE_AUTO_1_SHOT = 0x00,
    MODE_SLOW_1_SHOT = 0x01,
    MODE_FAST_1_SHOT = 0x02,
    MODE_AUTO_CONTINUOUS = 0x04,
    MODE_SLOW_CONTINUOUS = 0x05,
    MODE_FAST_CONTINUOUS = 0x06
} Laser_Mode;

// Structure to hold a single measurement
typedef struct
{
    float distance;            // Distance in meters
    Laser_Status status;       // Status of this measurement
    struct timespec timestamp; // When the measurement was taken
} Measurement;

// Circular buffer for measurements
typedef struct
{
    Measurement data[MAX_MEASUREMENTS];
    int head;
    int tail;
    int count;
} MeasurementBuffer;

// Structure for each laser sensor
typedef struct
{
    int fd;                         // File descriptor for serial port
    uint8_t address;                // Device address
    const char *device_path;        // UART device path
    MeasurementBuffer measurements; // Circular buffer of measurements
    float last_valid_distance;      // Last valid measurement
    time_t last_measurement_time;   // Timestamp of last measurement
    Laser_Status last_status;       // Last status read
} Laser_Device;

// Function prototypes
static uint8_t calculate_checksum(uint8_t *data, int length);
static int validate_response(uint8_t *response, int length);
static void init_measurement_buffer(MeasurementBuffer *buffer);
static void add_measurement(Laser_Device *device, float distance, Laser_Status status);

// Initialize the measurement buffer
static void init_measurement_buffer(MeasurementBuffer *buffer)
{
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
}

// Add a measurement to the circular buffer
static void add_measurement(Laser_Device *device, float distance, Laser_Status status)
{
    MeasurementBuffer *buffer = &device->measurements;

    // Get current timestamp
    Measurement new_measurement;
    clock_gettime(CLOCK_REALTIME, &new_measurement.timestamp);
    new_measurement.distance = distance;
    new_measurement.status = status;

    // Add to circular buffer
    buffer->data[buffer->head] = new_measurement;
    buffer->head = (buffer->head + 1) % MAX_MEASUREMENTS;

    if (buffer->count < MAX_MEASUREMENTS)
    {
        buffer->count++;
    }
    else
    {
        buffer->tail = (buffer->tail + 1) % MAX_MEASUREMENTS;
    }

    // Update last valid measurement if status is OK
    if (status == STATUS_OK)
    {
        device->last_valid_distance = distance;
    }

    device->last_status = status;
    device->last_measurement_time = new_measurement.timestamp.tv_sec;
}

// Initialize a laser rangefinder
int Laser_Init(Laser_Device *device)
{
    struct termios tty;

    // Wait for auto baudrate stage to timeout 2.5s
    usleep(INIT_DELAY_US);

    // Open serial port
    device->fd = open(device->device_path, O_RDWR | O_NOCTTY | O_NDELAY);
    if (device->fd < 0)
    {
        fprintf(stderr, "Failed to open serial port %s: %s\n", device->device_path, strerror(errno));
        return -1;
    }

    // Get current serial port settings
    if (tcgetattr(device->fd, &tty) != 0)
    {
        fprintf(stderr, "Failed to get serial port attributes: %s\n", strerror(errno));
        close(device->fd);
        return -1;
    }

    // Clear all settings
    memset(&tty, 0, sizeof(tty));

    // Set input and output baud rate
    if (cfsetispeed(&tty, BAUD_RATE) < 0 || cfsetospeed(&tty, BAUD_RATE) < 0)
    {
        fprintf(stderr, "Failed to set baud rate: %s\n", strerror(errno));
        close(device->fd);
        return -1;
    }

    // Control modes
    tty.c_cflag |= CS8;     // 8 data bits
    tty.c_cflag &= ~PARENB; // No parity
    tty.c_cflag &= ~CSTOPB; // 1 stop bit
    tty.c_cflag |= CLOCAL;  // Ignore modem status lines
    tty.c_cflag |= CREAD;   // Enable receiver

    // Input modes
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);

    // Output modes
    tty.c_oflag &= ~OPOST; // Raw output

    // Local modes
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    // Read settings
    tty.c_cc[VMIN] = 0;  // Return even if no chars received
    tty.c_cc[VTIME] = 5; // Wait up to 0.5 seconds

    // Flush port
    tcflush(device->fd, TCIOFLUSH);

    // Apply settings
    if (tcsetattr(device->fd, TCSANOW, &tty) != 0)
    {
        fprintf(stderr, "Failed to set serial port attributes: %s\n", strerror(errno));
        close(device->fd);
        return -1;
    }

    // Set blocking mode
    fcntl(device->fd, F_SETFL, 0);

    // Initialize device parameters
    device->address = DEFAULT_SLAVE_ADDR;
    device->last_valid_distance = 0.0f;
    device->last_status = STATUS_OK;
    device->last_measurement_time = 0;
    init_measurement_buffer(&device->measurements);

    return 0;
}

int Stop_Continuous(Laser_Device *device)
{
    uint8_t cmd[] = {0x58};
    if (write(device->fd, cmd, sizeof(cmd)) != sizeof(cmd))
    {
        fprintf(stderr, "Failed to write to sensor to stop continuous measure 1\n");
        return -1;
    }
    return 0;
}

// Perform single measurement
// int Laser_Measure(Laser_Device *device, Laser_Mode mode, float *distance)
// {
//     uint8_t cmd[] = {
//         0xAA, // Header
//         0x00, // Write command + slave address (0x00 write is 0 and read is 1) and (0x00 default slave address)
//         // device->address,        // Slave address
//         0x00, 0x20, // Register (MEA_START)
//         0x00, 0x01, // Payload count
//         0x00, mode, // Mode
//         0x21        // Checksum (calculated below)
//     };

//     // cmd[9] = calculate_checksum(&cmd[2], 7);

//     // Send command
//     if (write(device->fd, cmd, sizeof(cmd)) != sizeof(cmd))
//     {
//         fprintf(stderr, "Failed to write to sensor 1\n");
//         return -1;
//     }
    

//     // Read response
//     // uint8_t response[12];
//     // int bytes_read = 0;
//     // int timeout = 50; // 5 second timeout (100ms intervals)

//     // while (bytes_read < 12 && timeout > 0) {
//     //     int n = read(device->fd, &response[bytes_read], 12 - bytes_read);
//     //     if (n > 0) {
//     //         bytes_read += n;
//     //     }
//     //     usleep(100000);  // 100ms delay
//     //     timeout--;
//     // }
//     uint8_t reply;
//     int bytes_read = 0;
//     int timeout = 50; // 5 seconds (100ms intervals)

//     while (bytes_read < 12 && timeout > 0)
//     {
//         int n = read(device->fd, &reply, 1); // Read one byte at a time
//         if (n > 0)
//         {
//             printf("Byte %d: 0x%02X\n", bytes_read, reply); // Print raw byte
//             bytes_read++;
//         }
//         usleep(100000); // 100ms delay
//         timeout--;
//     }

//     if (bytes_read != 12)
//     {
//         fprintf(stderr, "Error: Expected 12 bytes, got %d\n", bytes_read);
//         return -1;
//     }

//     // Print raw response
//     // fprintf(stderr, "Raw Sensor Response: ");
//     // for (int i = 0; i < bytes_read; i++) {
//     //     fprintf(stderr, "%02X ", response[i]);
//     // }
//     // fprintf(stderr, "\n");

//     // if (bytes_read != 12)
//     // {
//     //     // add_measurement(device, 0.0f, STATUS_INVALID_FRAME);
//     //     fprintf(stderr, "Invalid frame from sensor, expected 12 bytes but got %d\n", bytes_read);
//     //     return -1;
//     // }

//     // Validate response
//     // if (!validate_response(response, bytes_read))
//     // {
//     //     // add_measurement(device, 0.0f, STATUS_INVALID_FRAME);
//     //     fprintf(stderr, "Invalid frame from sensor, checksum mismatch 1\n");
//     //     return -2;
//     // }

//     // Extract distance value (4 bytes starting at index 6)
//     // uint32_t dist_mm = (response[6] << 24) | (response[7] << 16) |
//     //                    (response[8] << 8) | response[9];
//     // *distance = dist_mm / 1000.0f; // Convert to meters

//     // Store measurement
//     // add_measurement(device, *distance, STATUS_OK);

//     return 0;
// }

// perform measurement after writing auto 1 shot command frame and then writing read measurement command
int Laser_Measure(Laser_Device *device, Laser_Mode mode, float *distance) {
    uint8_t cmd[] = {
        0xAA,  // Header
        0x00,  // Address + Write (7-bit address 0x00 + write bit 0)
        0x00, 0x20,  // Register (MEA_START)
        0x00, 0x01,  // Payload count (1 byte)
        0x00, mode,  // Mode (0x00 = Auto 1-shot)
        0x21  // Checksum (precomputed, should be recalculated if dynamic)
    };

    // Send measurement start command
    if (write(device->fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        fprintf(stderr, "Error: Failed to send measurement command\n");
        return -1;
    }

    // Wait for measurement to complete (based on manual: ~0.4 - 4s)
    usleep(1000000); // 500ms delay (adjust as needed)

    // Read measurement result command
    uint8_t read_cmd[] = { 0xAA, 0x80, 0x00, 0x22, 0xA2 };
    if (write(device->fd, read_cmd, sizeof(read_cmd)) != sizeof(read_cmd)) {
        fprintf(stderr, "Error: Failed to request measurement result\n");
        return -1;
    }

    // Read response
    uint8_t response[12];
    int bytes_read = read(device->fd, response, sizeof(response));

    if (bytes_read != 12) {
        fprintf(stderr, "Error: Invalid response size (%d bytes received)\n", bytes_read);
        return -1;
    }

    // Validate response
    if (response[0] != 0xAA || response[1] != 0x80 || response[2] != 0x00 || response[3] != 0x22) {
        fprintf(stderr, "Error: Unexpected response header\n");
        return -2;
    }

    // Extract distance (4 bytes at index 6-9)
    uint32_t dist_mm = (response[6] << 24) | (response[7] << 16) |
                       (response[8] << 8)  | response[9];

    *distance = dist_mm / 1000.0f; // Convert mm to meters

    printf("Measured Distance: %.2f m\n", *distance);
    return 0;
}

// perform module status check
int Laser_ReadStatus(Laser_Device *device) {
    uint8_t cmd[] = { 
        0xAA,  // Header
        0x80,  // R/W + Address (0x00 | 0x80 for Read)
        0x00, 0x00,  // Register (Module Status Register: 0x0000)
        0x80  // Checksum
    };

    // Send the Read Status command
    int bytes_written = write(device->fd, cmd, sizeof(cmd));
    if (bytes_written != sizeof(cmd)) {
        fprintf(stderr, "Error: Failed to send status request. Sent %d bytes instead of %ld\n",
                bytes_written, sizeof(cmd));
        return -1;
    }

    // Read response from the sensor
    uint8_t response[8];  
    int bytes_read = read(device->fd, response, sizeof(response));

    if (bytes_read != 8) {
        fprintf(stderr, "Error: Invalid response size (%d bytes received)\n", bytes_read);
        return -1;
    }

    // Print the raw response
    printf("Raw Status Response: ");
    for (int i = 0; i < bytes_read; i++) {
        printf("%02X ", response[i]);
    }
    printf("\n");

    // Validate response structure
    if (response[0] != 0xAA || response[1] != 0x80 || response[2] != 0x00 || response[3] != 0x00) {
        fprintf(stderr, "Error: Unexpected response format\n");
        return -2;
    }

    // Extract status code (Byte 6)
    uint16_t status_code = response[6];

    printf("Module Status Code: 0x%04X\n", status_code);
    
    return status_code;  // Return status code for further debugging
}


// Example usage function
void init_both_sensors(Laser_Device *sensor1, Laser_Device *sensor2)
{
    // Initialize sensor 1 on UART2
    sensor1->device_path = UART2_DEVICE;
    if (Laser_Init(sensor1) != 0)
    {
        fprintf(stderr, "Failed to initialize sensor 1\n");
        exit(1);
    }

    // Initialize sensor 2 on UART3
    sensor2->device_path = UART3_DEVICE;
    if (Laser_Init(sensor2) != 0)
    {
        fprintf(stderr, "Failed to initialize sensor 2\n");
        exit(1);
    }
}

// Helper function to get measurement statistics
void get_measurement_stats(Laser_Device *device, float *avg, float *min, float *max)
{
    MeasurementBuffer *buffer = &device->measurements;
    if (buffer->count == 0)
    {
        *avg = *min = *max = 0;
        return;
    }

    float sum = 0;
    *min = 999999;
    *max = -999999;
    int valid_count = 0;

    for (int i = 0; i < buffer->count; i++)
    {
        int idx = (buffer->tail + i) % MAX_MEASUREMENTS;
        if (buffer->data[idx].status == STATUS_OK)
        {
            float dist = buffer->data[idx].distance;
            sum += dist;
            *min = (dist < *min) ? dist : *min;
            *max = (dist > *max) ? dist : *max;
            valid_count++;
        }
    }

    *avg = valid_count > 0 ? sum / valid_count : 0;
}

// Calculate checksum
static uint8_t calculate_checksum(uint8_t *data, int length)
{
    uint8_t sum = 0;
    for (int i = 0; i < length; i++)
    {
        sum += data[i];
    }
    return sum;
}

// Validate response frame
static int validate_response(uint8_t *response, int length)
{
    if (length < 3)
        return 0;

    // Check header
    if (response[0] != 0xAA && response[0] != 0xEE)
        return 0;

    // Calculate and verify checksum
    uint8_t calc_checksum = calculate_checksum(&response[1], length - 2);
    return (calc_checksum == response[length - 1]);
}

// turn on or off laser beam