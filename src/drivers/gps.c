#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

/*
NMEA0183 V4.1 Protocol used by GROVE GPS 350

https://en.wikipedia.org/wiki/NMEA_0183

GGA: time, location, num of satellites
GSA: GPS receiver operating mode, satellites used for positioning, DOP (Dilution of precision) value, positioning status
GSV: visible GPS satellite information, elevation angle, azimuth angle, signal-to-noise ratio
RMC: time, data, location, speed
VTG: ground speed information



*/


#define GPS_BAUD_RATE B9600
#define UART_DEVICE "/dev/serial0"
#define QUEUE_SIZE 500
#define GPS_BUFFER_SIZE 64

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
static char buffer[GPS_BUFFER_SIZE];
static int uart_fd = -1;

void parse_gps_data(GPSData* input, char* buffer){
    return;
}

// Queue functions remain unchanged
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

void init_gps_data(GPSData *data) {
    memset(data, 0, sizeof(GPSData));
}

int read_gps_data(char *buffer, size_t buffer_size) {
    int bytesRead = read(uart_fd, buffer, buffer_size - 1);
    if (bytesRead < 0) {
        //fprintf(stderr, "Error reading from UART: %s\n", strerror(errno));
        return -1;
    }
    buffer[bytesRead] = '\0'; // Null-terminate the buffer
    return bytesRead;
}

void gps_init() {
    // Open UART device
    uart_fd = open(UART_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd < 0) {
        fprintf(stderr, "Error opening %s: %s\n", UART_DEVICE, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Configure UART
    struct termios uart_config;
    memset(&uart_config, 0, sizeof(uart_config));

    // Get current configuration
    if (tcgetattr(uart_fd, &uart_config) != 0) {
        fprintf(stderr, "Error getting UART configuration: %s\n", strerror(errno));
        close(uart_fd);
        exit(EXIT_FAILURE);
    }

    // Set baud rate
    cfsetispeed(&uart_config, GPS_BAUD_RATE);
    cfsetospeed(&uart_config, GPS_BAUD_RATE);

    // Set 8N1 (8 bits, no parity, 1 stop bit)
    uart_config.c_cflag &= ~PARENB;  // No parity
    uart_config.c_cflag &= ~CSTOPB;  // 1 stop bit
    uart_config.c_cflag &= ~CSIZE;   // Clear size bits
    uart_config.c_cflag |= CS8;      // 8 bits
    uart_config.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem controls

    // Set input mode (non-canonical, no echo,...)
    uart_config.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    uart_config.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable software flow control
    uart_config.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    // Set output mode
    uart_config.c_oflag &= ~OPOST;

    // Set read timeout
    uart_config.c_cc[VMIN] = 0;
    uart_config.c_cc[VTIME] = 10; // 1 second timeout

    // Apply configuration
    if (tcsetattr(uart_fd, TCSANOW, &uart_config) != 0) {
        fprintf(stderr, "Error setting UART configuration: %s\n", strerror(errno));
        close(uart_fd);
        exit(EXIT_FAILURE);
    }

    // Clear input buffer
    tcflush(uart_fd, TCIFLUSH);

    printf("Opened UART device: %s at baud rate: %d\n", UART_DEVICE, B9600);
    
    init_gps_data(&gps_data);
    init_queue();
}

void spin() {
    int bytesRead = read_gps_data(buffer, GPS_BUFFER_SIZE);
    if (bytesRead > 0) {
        parse_gps_data(&gps_data, buffer);
        enqueue(&gps_data);
    }
}

void test_read() {
    int bytesRead = read_gps_data(buffer, GPS_BUFFER_SIZE);
    if (bytesRead > 0) {
        printf("%d bytes read\n%s\n", bytesRead, buffer);
    }
}

void terminate_connection() {
    if (uart_fd >= 0) {
        close(uart_fd);
        uart_fd = -1;
    }
}