#ifndef GPS_H
#define GPS_H

#include <stdint.h>

#define QUEUE_SIZE 500
#define GPS_BAUD_RATE 9600
#define GPS_UART_PORT 1

typedef struct {
    double latitude;
    double longitude;
    double height;
    double utc_time;
    double velocity_x;
    double velocity_y;
    double velocity_z;
    double accel_x;
    double accel_y;
    double accel_z;
} GPSData;

typedef struct {
    GPSData data[QUEUE_SIZE];
    int head;
    int tail;
    int count;
} CircularQueue;

// Queue operations
void init_queue(void);
void enqueue(const GPSData *data);
GPSData* dequeue(void);
void print_queue(void);

// GPS operations
void gps_init(void);
int read_gps_data(char *buffer, size_t buffer_size);
void parse_nmea_sentence(GPSData* data, const char* sentence);
void print_gps_data(const GPSData *data);
GPSData* spin_gps(void);
void test_read(void);
void terminate_connection(void);
void test_final_values(void);

#endif // GPS_H