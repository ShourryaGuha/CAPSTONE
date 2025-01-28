#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pigpio.h>

// relevant pigpio api: https://abyz.me.uk/rpi/pigpio/cif.html#gpioSerialRead

#define GPS_BAUD_RATE 9600
#define GPS_UART_PORT 1
#define QUEUE_SIZE 500

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

static CircularQueue queue;
static GPSData gps_data;
static char buffer[256];

void init_queue() {
    queue.head = 0;
    queue.tail = 0;
    queue.count = 0;
}

void enqueue(const GPSData *data) {
    if (queue.count == QUEUE_SIZE) {
        queue.head = (queue.head + 1) % QUEUE_SIZE;
        queue.count--;
    }
    queue.data[queue.tail] = *data;
    queue.tail = (queue.tail + 1) % QUEUE_SIZE;
    queue.count++;
}

GPSData* dequeue() {
    if (queue.count == 0) return NULL;
    static GPSData data;
    data = queue.data[queue.head];
    queue.head = (queue.head + 1) % QUEUE_SIZE;
    queue.count--;
    return &data;
}

void print_queue() {
    int current = queue.head;
    for (int i = 0; i < queue.count; i++) {
        printf("State %d: Lat: %.6f, Lon: %.6f, Height: %.2f\n",
               i + 1, queue.data[current].latitude,
               queue.data[current].longitude,
               queue.data[current].height);
        current = (current + 1) % QUEUE_SIZE;
    }
    printf("\n");
}

void init_gps_data(GPSData *data) {
    memset(data, 0, sizeof(GPSData));
}

int read_gps_data(char *buffer, size_t buffer_size) {
    int bytesRead = 0;
    int i = 0;
    char c;
    while (bytesRead < buffer_size - 1) {
        if (gpioSerialRead(GPS_UART_PORT, &c, 256) == -1) 
            break;
        buffer[i++] = c;
        bytesRead++;
        usleep(10000);
    }
    buffer[i] = '\0';
    return bytesRead;
}

void parse_gps_data(GPSData *data, const char *str) {
    data->latitude = 37.7749;
    data->longitude = -122.4194;
    data->utc_time = 123456.78;
}

void print_gps_data(const GPSData *data) {
    printf("GPS Data:\n");
    printf("Latitude: %.6f\n", data->latitude);
    printf("Longitude: %.6f\n", data->longitude);
    printf("Height: %.2f m\n", data->height);
    printf("UTC Time: %.2f\n", data->utc_time);
    printf("Velocity: [X: %.2f, Y: %.2f, Z: %.2f] m/s\n",
           data->velocity_x, data->velocity_y, data->velocity_z);
    printf("Acceleration: [X: %.2f, Y: %.2f, Z: %.2f] m/s²\n",
           data->accel_x, data->accel_y, data->accel_z);
}

void gps_init() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialization failed.\n");
        return;
    }
    if (gpioSerialReadOpen(GPS_UART_PORT, GPS_BAUD_RATE, 8) < 0) {
        fprintf(stderr, "Failed to open UART for GPS\n");
        gpioTerminate();
        return;
    }
    init_gps_data(&gps_data);
    init_queue();
}

void spin() {
    int bytesRead = read_gps_data(buffer, sizeof(buffer));
    if (bytesRead > 0) {
        parse_gps_data(&gps_data, buffer);
        enqueue(&gps_data);
    }
}

void test_read() {
    int bytesRead = read_gps_data(buffer, sizeof(buffer));
    if (bytesRead > 0) {
        print_gps_data(&gps_data);
    }
}

void terminate_connection() {
    gpioSerialReadClose(GPS_UART_PORT);
    gpioTerminate();
}

// int main() {
//     gps_init();
//     while(1) {
//         test_read();
//         usleep(10000);
//     }
//     terminate_connection();
//     return 0;
// }