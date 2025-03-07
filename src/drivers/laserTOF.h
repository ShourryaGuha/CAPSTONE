#ifndef LASERTOF_H
#define LASERTOF_H

#include <stdint.h>
#include <time.h>

// Constants
#define UART2_DEVICE "/dev/ttyAMA2"  //confirmed it corresponds to uart2 gpio 0 and 1   // J3 on pi
#define UART3_DEVICE "/dev/ttyAMA3"   //confirmed it corresponds to uart3 gpio 4 and 5   // J1 on pi
#define BAUD_RATE B115200
#define MAX_BUFFER_SIZE 32
#define DEFAULT_SLAVE_ADDR 0x00
#define INIT_DELAY_US 2500000
#define MAX_MEASUREMENTS 100

// Measurement structure
typedef struct {
    float distance;
    struct timespec timestamp;
} Measurement;

// Circular buffer for measurements
typedef struct {
    Measurement data[MAX_MEASUREMENTS];
    int head;
    int tail;
    int count;
} MeasurementBuffer;


typedef struct
{
    int fd;                         // File descriptor for serial port
    uint8_t address;                // Device address
    const char *device_path;        // UART device path
    // MeasurementBuffer measurements; // Circular buffer of measurements
    // float last_valid_distance;      // Last valid measurement
    // time_t last_measurement_time;   // Timestamp of last measurement
    // Laser_Status last_status;       // Last status read
    float distance;  //measured distance
} Laser_Device;

// Function prototypes
int Laser_Init(Laser_Device *device);
int Laser_Measure_Single(Laser_Device *device);
int Laser_stop_continuous(Laser_Device *device);
int Laser_Measure_Continuous(Laser_Device *device);
// int Laser_ReadStatus(Laser_Device *device);
// int Stop_Continuous(Laser_Device *device);
// void init_both_sensors(Laser_Device *sensor1, Laser_Device *sensor2);
// void get_measurement_stats(Laser_Device *device, float *avg, float *min, float *max);



#endif /* LASERTOF_H */

