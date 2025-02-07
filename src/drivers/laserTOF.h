#ifndef LASERTOF_H
#define LASERTOF_H

#include <stdint.h>
#include <time.h>

// Constants
#define UART2_DEVICE "/dev/ttyAMA2"
#define UART3_DEVICE "/dev/ttyAMA3"
#define BAUD_RATE B19200
#define MAX_BUFFER_SIZE 32
#define DEFAULT_SLAVE_ADDR 0x00
#define INIT_DELAY_US 2500000
#define MAX_MEASUREMENTS 100

// Status codes
typedef enum {
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
typedef enum {
    MODE_AUTO_1_SHOT = 0x00,
    MODE_SLOW_1_SHOT = 0x01,
    MODE_FAST_1_SHOT = 0x02,
    MODE_AUTO_CONTINUOUS = 0x04,
    MODE_SLOW_CONTINUOUS = 0x05,
    MODE_FAST_CONTINUOUS = 0x06
} Laser_Mode;

// Measurement structure
typedef struct {
    float distance;
    Laser_Status status;
    struct timespec timestamp;
} Measurement;

// Circular buffer for measurements
typedef struct {
    Measurement data[MAX_MEASUREMENTS];
    int head;
    int tail;
    int count;
} MeasurementBuffer;

// Laser device structure
typedef struct {
    int fd;
    uint8_t address;
    const char* device_path;
    MeasurementBuffer measurements;
    float last_valid_distance;
    time_t last_measurement_time;
    Laser_Status last_status;
} Laser_Device;

// Function prototypes
int Laser_Init(Laser_Device *device);
int Laser_Measure(Laser_Device *device, Laser_Mode mode, float *distance);
int Laser_ReadStatus(Laser_Device *device);
int Stop_Continuous(Laser_Device *device);
void init_both_sensors(Laser_Device *sensor1, Laser_Device *sensor2);
void get_measurement_stats(Laser_Device *device, float *avg, float *min, float *max);



#endif /* LASERTOF_H */

