#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <math.h>
#include <time.h>

/*
NMEA0183 V4.1 Protocol used by GROVE GPS 350

https://en.wikipedia.org/wiki/NMEA_0183

GGA: time, location, num of satellites
GSA: GPS receiver operating mode, satellites used for positioning, DOP (Dilution of precision) value, positioning status
GSV: visible GPS satellite information, elevation angle, azimuth angle, signal-to-noise ratio
RMC: time, data, location, speed
VTG: ground speed information

*/

//reading serial
#define GPS_BAUD_RATE B9600
#define UART_DEVICE "/dev/serial0"
#define QUEUE_SIZE 500
#define GPS_BUFFER_SIZE 64

//velocity calculations
#define KNOTS_TO_MPS 0.514444
#define EARTH_RADIUS 6371000.0  // Earth radius in meters
#define MIN_SATELLITES_FOR_FIX 4 //3 for each dimension, 1 more for error checking
#ifndef M_PI
#   define M_PI 3.1415926535897932384626433832
#endif


// Structure to store previous measurements for acceleration calculation
static struct {
    double prev_vel_x;
    double prev_vel_y;
    double prev_vel_z;
    double prev_time;
    double prev_height;
    int initialized;
} prev_state = {0, 0, 0, 0, 0, 0};

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


// Helper function to convert NMEA coordinate format to decimal degrees
double nmea_to_decimal(const char* coord, char dir) {
    if (!coord[0]) return 0.0;  // Empty field
    
    double degrees = atof(coord) / 100.0;
    double minutes = degrees - floor(degrees);
    degrees = floor(degrees);
    double decimal = degrees + (minutes * 100.0) / 60.0;
    
    return (dir == 'S' || dir == 'W') ? -decimal : decimal;
}

// Convert HHMMSS.ss to seconds since midnight
double parse_utc_time(const char* time_str) {
    if (!time_str[0]) return 0.0;  // Empty field
    
    int hours = (time_str[0] - '0') * 10 + (time_str[1] - '0');
    int minutes = (time_str[2] - '0') * 10 + (time_str[3] - '0');
    double seconds = atof(time_str + 4);
    
    return hours * 3600.0 + minutes * 60.0 + seconds;
}

void find_velocity(GPSData* data, double speed_knots, double course) {
    // Convert speed from knots to m/s and course to radians
    double speed_ms = speed_knots * KNOTS_TO_MPS;
    double course_rad = course * M_PI / 180.0;
    
    // Calculate velocity components
    data->velocity_x = speed_ms * sin(course_rad);  // East component
    data->velocity_y = speed_ms * cos(course_rad);  // North component

    // Calculate vertical velocity (change in height over time)
    if (prev_state.initialized) {
        double dt = data->utc_time - prev_state.prev_time;
        if (dt > 0) {
            data->velocity_z = (data->height - prev_state.prev_height) / dt;
        }
    } else {
        data->velocity_z = 0.0;
    }

    // Store previous values
    prev_state.prev_height = data->height;
}

void find_acceleration(GPSData* data) {
    if (!prev_state.initialized) {
        prev_state.prev_vel_x = data->velocity_x;
        prev_state.prev_vel_y = data->velocity_y;
        prev_state.prev_vel_z = data->velocity_z;
        prev_state.prev_time = data->utc_time;
        prev_state.initialized = 1;
        return;
    }
    
    double dt = data->utc_time - prev_state.prev_time;
    if (dt > 0) {
        // Calculate acceleration components
        data->accel_x = (data->velocity_x - prev_state.prev_vel_x) / dt;
        data->accel_y = (data->velocity_y - prev_state.prev_vel_y) / dt;
        data->accel_z = (data->velocity_z - prev_state.prev_vel_z) / dt;
        
        // Update previous state
        prev_state.prev_vel_x = data->velocity_x;
        prev_state.prev_vel_y = data->velocity_y;
        prev_state.prev_vel_z = data->velocity_z;
        prev_state.prev_time = data->utc_time;
    }
}

void parse_gps_data(GPSData* data, char* buffer) {
    char* line = strtok(buffer, "\n");
    double speed_knots = 0.0;
    double course = 0.0;
    int satellites = 0;
    int fix_quality = 0;
    int rmc_valid = 0;  // Flag for RMC validity
    
    while (line != NULL) {
        if (strncmp(line, "$GNGGA", 6) == 0 || strncmp(line, "$GPGGA", 6) == 0) {
            char* field = strtok(line, ",");
            int field_count = 0;
            
            while (field != NULL) {
                switch(field_count) {
                    case 1:  // UTC time
                        data->utc_time = parse_utc_time(field);
                        break;
                    case 2:  // Latitude
                        data->latitude = nmea_to_decimal(field, *(field + 12));
                        break;
                    case 4:  // Longitude
                        data->longitude = nmea_to_decimal(field, *(field + 12));
                        break;
                    case 6:  // Fix quality
                        fix_quality = field[0] ? atoi(field) : 0;
                        break;
                    case 7:  // Number of satellites
                        satellites = field[0] ? atoi(field) : 0;
                        break;
                    case 9:  // Altitude
                        data->height = field[0] ? atof(field) : 0.0;
                        break;
                }
                field = strtok(NULL, ",");
                field_count++;
            }
        }
        else if (strncmp(line, "$GNRMC", 6) == 0 || strncmp(line, "$GPRMC", 6) == 0) {
            char* field = strtok(line, ",");
            int field_count = 0;
            
            while (field != NULL) {
                switch(field_count) {
                    case 2:  // Validity flag
                        rmc_valid = (field[0] == 'A');  // 'A' for active, 'V' for void
                        break;
                    case 7:  // Speed in knots
                        speed_knots = field[0] ? atof(field) : 0.0;
                        break;
                    case 8:  // Course over ground
                        course = field[0] ? atof(field) : 0.0;
                        break;
                }
                field = strtok(NULL, ",");
                field_count++;
            }
        }
        line = strtok(NULL, "\n");
    }
    
    // Only process data if we have both a valid RMC fix and enough satellites
    if (rmc_valid && fix_quality > 0 && satellites >= MIN_SATELLITES_FOR_FIX) {
        find_velocity(data, speed_knots, course);
        find_acceleration(data);
    } else {
        // If no valid fix, zero out all dynamic data
        data->velocity_x = 0.0;
        data->velocity_y = 0.0;
        data->velocity_z = 0.0;
        data->accel_x = 0.0;
        data->accel_y = 0.0;
        data->accel_z = 0.0;
        
        // Zero out position data as well since it's not reliable
        data->latitude = 0.0;
        data->longitude = 0.0;
        data->height = 0.0;
        
        // fprintf(stderr, "Invalid GPS fix - RMC Valid: %d, Fix Quality: %d, Satellites: %d\n",
                // rmc_valid, fix_quality, satellites);
    }
}

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

    printf("Opened UART device: %s at baud rate: %d\n", UART_DEVICE, 9600);
    
    init_gps_data(&gps_data);
    init_queue();
}

GPSData* spin_gps() {
    int bytesRead = read_gps_data(buffer, GPS_BUFFER_SIZE);
    if (bytesRead > 0) {
        parse_gps_data(&gps_data, buffer);
        enqueue(&gps_data);
    }
    return &gps_data;
}

void test_read() {
    int bytesRead = read_gps_data(buffer, GPS_BUFFER_SIZE);
    if (bytesRead > 0) {
        printf("%d bytes read\n%s\n", bytesRead, buffer);
    }
}

void test_final_values(){
    printf("GPS Data Values:\n");
    printf("Position:\n");
    printf("  Latitude:  %f°\n", gps_data.latitude);
    printf("  Longitude: %f°\n", gps_data.longitude);
    printf("  Height:    %f m\n", gps_data.height);
    printf("\nTime:\n");
    printf("  UTC Time:  %f seconds\n", gps_data.utc_time);
    printf("\nVelocity (m/s):\n");
    printf("  X: %f\n", gps_data.velocity_x);
    printf("  Y: %f\n", gps_data.velocity_y);
    printf("  Z: %f\n", gps_data.velocity_z);
    printf("\nAcceleration (m/s²):\n");
    printf("  X: %f\n", gps_data.accel_x);
    printf("  Y: %f\n", gps_data.accel_y);
    printf("  Z: %f\n", gps_data.accel_z);
    printf("\n");
}

void terminate_connection() {
    if (uart_fd >= 0) {
        close(uart_fd);
        uart_fd = -1;
    }
}