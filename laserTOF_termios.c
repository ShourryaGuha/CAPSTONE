#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <stdint.h> // Added for uint8_t

// Pin definitions removed since we're not using GPIO directly

// Constants
#define BAUD_RATE B19200 // Using termios baud rate constant
#define MAX_BUFFER_SIZE 32
#define DEFAULT_SLAVE_ADDR 0x00

// Status codes from manual
typedef enum
{
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
typedef enum
{
    MODE_AUTO = 0x00,
    MODE_SLOW = 0x01,
    MODE_FAST = 0x02
} Laser_Mode;

typedef struct
{
    int fd;          // File descriptor for serial port
    uint8_t address; // Device address
} Laser_Device;

// Function prototypes
static uint8_t calculate_checksum(uint8_t *data, int length);
static int validate_response(uint8_t *response, int length);

// Initialize the laser rangefinder
int Laser_Init(Laser_Device *device)
{
    struct termios tty;

    // Open serial port
    device->fd = open("/dev/ttyAMA3", O_RDWR | O_NOCTTY | O_SYNC);
    if (device->fd < 0)
    {
        fprintf(stderr, "Failed to open serial port: %s\n", strerror(errno));
        return -1;
    }

    // Get current serial port settings
    if (tcgetattr(device->fd, &tty) != 0)
    {
        fprintf(stderr, "Failed to get serial port attributes: %s\n", strerror(errno));
        close(device->fd);
        return -1;
    }

    // Set serial port settings
    cfsetospeed(&tty, BAUD_RATE);
    cfsetispeed(&tty, BAUD_RATE);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                     // Disable break processing
    tty.c_lflag = 0;                            // No signaling chars, no echo
    tty.c_oflag = 0;                            // No remapping, no delays
    tty.c_cc[VMIN] = 0;                         // Read doesn't block
    tty.c_cc[VTIME] = 5;                        // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);        // Ignore modem controls
    tty.c_cflag &= ~(PARENB | PARODD);      // Shut off parity
    tty.c_cflag &= ~CSTOPB;
#ifdef CRTSCTS
    tty.c_cflag &= ~CRTSCTS; // Disable hardware flow control
#endif

    // Apply settings
    if (tcsetattr(device->fd, TCSANOW, &tty) != 0)
    {
        fprintf(stderr, "Failed to set serial port attributes: %s\n", strerror(errno));
        close(device->fd);
        return -1;
    }

    device->address = DEFAULT_SLAVE_ADDR;

    // Auto baud rate detection
    uint8_t autobaud = 0x55;
    if (write(device->fd, &autobaud, 1) != 1)
    {
        fprintf(stderr, "Failed to write autobaud byte: %s\n", strerror(errno));
        close(device->fd);
        return -1;
    }

    usleep(100000); // Wait for reply

    uint8_t reply;
    if (read(device->fd, &reply, 1) != 1)
    {
        if (read(device->fd, &reply, 1) == 0)
        {
            fprintf(stderr, "End of file or no data available.\n");
            close(device->fd);
            return -1;
        }
        else if (read(device->fd, &reply, 1) < 0)
        {
            perror("read error");
            close(device->fd);
            return -1;
        }
    }

    return 0;
}

// Perform single measurement
int Laser_Measure(Laser_Device *device, Laser_Mode mode, float *distance)
{
    uint8_t cmd[] = {
        0xAA,            // Header
        0x00,            // Write command
        device->address, // Slave address
        0x00, 0x20,      // Register (MEA_START)
        0x00, 0x01,      // Payload count
        0x00, mode,      // Mode
        0x00             // Checksum (calculated below)
    };

    cmd[9] = calculate_checksum(&cmd[2], 7);

    // Send command
    if (write(device->fd, cmd, sizeof(cmd)) != sizeof(cmd))
    {
        return -1;
    }

    // Read response
    uint8_t response[12];
    int bytes_read = 0;
    int timeout = 50; // 5 second timeout (100ms intervals)

    while (bytes_read < 12 && timeout > 0)
    {
        int n = read(device->fd, &response[bytes_read], 12 - bytes_read);
        if (n > 0)
        {
            bytes_read += n;
        }
        usleep(100000); // 100ms delay
        timeout--;
    }

    if (bytes_read != 12)
    {
        return -1;
    }

    // Validate response
    if (!validate_response(response, bytes_read))
    {
        return -2;
    }

    // Extract distance value (4 bytes starting at index 6)
    uint32_t dist_mm = (response[6] << 24) | (response[7] << 16) |
                       (response[8] << 8) | response[9];
    *distance = dist_mm / 1000.0f; // Convert to meters

    return 0;
}

// Read device status
Laser_Status Laser_GetStatus(Laser_Device *device)
{
    uint8_t cmd[] = {
        0xAA,            // Header
        0x80,            // Read command
        device->address, // Slave address
        0x00, 0x00,      // Register (ERR_CODE)
        0x80             // Checksum
    };

    if (write(device->fd, cmd, sizeof(cmd)) != sizeof(cmd))
    {
        return STATUS_INVALID_FRAME;
    }

    uint8_t response[9];
    int bytes_read = read(device->fd, response, sizeof(response));

    if (bytes_read != 9)
    {
        return STATUS_INVALID_FRAME;
    }

    return (Laser_Status)((response[6] << 8) | response[7]);
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