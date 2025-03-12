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

// #define DEBUG

//reading serial
#define GPS_BAUD_RATE B9600
#define UART_DEVICE "/dev/serial0"
#define QUEUE_SIZE 500
#define GPS_BUFFER_SIZE 1024

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
static char buffer[GPS_BUFFER_SIZE] = {0};
static int buffer_pos = 0;
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

void parse_nmea_sentence(GPSData* data, const char* sentence) {
    char sentence_copy[128];
    strncpy(sentence_copy, sentence, sizeof(sentence_copy) - 1);
    sentence_copy[sizeof(sentence_copy) - 1] = '\0';  // Ensure null termination
    
    double speed_knots = 0.0;
    double course = 0.0;
    int fix_quality = 0;
    int satellites = 0;
    int valid = 0;
    
    #ifdef DEBUG
    printf("Processing NMEA: %s", sentence_copy);
    #endif
    
    // Check for GGA sentence (position and fix data)
    if (strncmp(sentence_copy, "$GNGGA", 6) == 0 || strncmp(sentence_copy, "$GPGGA", 6) == 0) {
        char* fields[15] = {NULL};  // GGA has 15 fields
        int field_count = 0;
        
        // Split the sentence into fields
        char* field = strtok(sentence_copy, ",");
        while (field != NULL && field_count < 15) {
            fields[field_count++] = field;
            field = strtok(NULL, ",");
        }
        
        // Process fields if we have enough
        if (field_count >= 10) {
            // UTC time
            if (fields[1] && *fields[1])
                data->utc_time = parse_utc_time(fields[1]);
            
            // Latitude
            if (fields[2] && *fields[2] && fields[3] && *fields[3])
                data->latitude = nmea_to_decimal(fields[2], *fields[3]);
            
            // Longitude
            if (fields[4] && *fields[4] && fields[5] && *fields[5])
                data->longitude = nmea_to_decimal(fields[4], *fields[5]);
            
            // Fix quality
            if (fields[6] && *fields[6])
                fix_quality = atoi(fields[6]);
            
            // Number of satellites
            if (fields[7] && *fields[7])
                satellites = atoi(fields[7]);
            
            // Altitude
            if (fields[9] && *fields[9])
                data->height = atof(fields[9]);
            
            // Update valid flag if fix quality is good and we have enough satellites
            if (fix_quality > 0 && satellites >= MIN_SATELLITES_FOR_FIX)
                valid = 1;
        }
    }
    // Check for RMC sentence (recommended minimum data)
    else if (strncmp(sentence_copy, "$GNRMC", 6) == 0 || strncmp(sentence_copy, "$GPRMC", 6) == 0) {
        char* fields[13] = {NULL};  // RMC has 13 fields
        int field_count = 0;
        
        // Split the sentence into fields
        char* field = strtok(sentence_copy, ",");
        while (field != NULL && field_count < 13) {
            fields[field_count++] = field;
            field = strtok(NULL, ",");
        }
        
        // Process fields if we have enough
        if (field_count >= 9) {
            // Status - 'A'=Active, 'V'=Void
            if (fields[2] && *fields[2])
                valid = (*fields[2] == 'A');
            
            // Speed in knots
            if (fields[7] && *fields[7])
                speed_knots = atof(fields[7]);
            
            // Course over ground
            if (fields[8] && *fields[8])
                course = atof(fields[8]);
            
            // Only update velocity and acceleration if data is valid
            if (valid) {
                find_velocity(data, speed_knots, course);
                find_acceleration(data);
            }
        }
    }
    
    // If no valid data, reset dynamic values
    if (!valid) {
        data->velocity_x = 0.0;
        data->velocity_y = 0.0;
        data->velocity_z = 0.0;
        data->accel_x = 0.0;
        data->accel_y = 0.0;
        data->accel_z = 0.0;
        // Don't reset position data on every invalid sentence
        // Only reset it if fix quality is explicitly bad
        // if (fix_quality == 0) {
        //     data->latitude = 0.0;
        //     data->longitude = 0.0;
        //     data->height = 0.0;
        // }
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

int read_gps_data() {
    char temp_buf[128];  // Temporary buffer for reading
    int bytes_read = read(uart_fd, temp_buf, sizeof(temp_buf) - 1);
    
    if (bytes_read <= 0) {
        return 0;
    }
    
    // Add new data to buffer
    if (buffer_pos + bytes_read >= GPS_BUFFER_SIZE) {
        // Buffer overflow, reset
        buffer_pos = 0;
    }
    
    memcpy(buffer + buffer_pos, temp_buf, bytes_read);
    buffer_pos += bytes_read;
    buffer[buffer_pos] = '\0';  // Null terminate
    
    return 1;  // Data was read
}

int extract_nmea_sentence(char *sentence, int max_len) {
    // Validate input parameters
    if (!sentence || max_len <= 0) {
        return 0;
    }
    
    // Clear the output buffer immediately
    sentence[0] = '\0';
    
    // Sanity check for buffer_pos being valid
    if (buffer_pos < 0 || buffer_pos >= GPS_BUFFER_SIZE) {
        // Something went wrong, reset the buffer
        buffer_pos = 0;
        return 0;
    }
    
    // Always ensure null termination
    buffer[buffer_pos] = '\0';
    
    // Find start of sentence
    int start_pos = -1;
    for (int i = 0; i < buffer_pos; i++) {
        if (buffer[i] == '$') {
            start_pos = i;
            break;
        }
    }
    
    if (start_pos == -1) {
        // No NMEA sentence start found
        buffer_pos = 0;  // Reset buffer
        return 0;
    }
    
    // Remove any data before the start marker
    if (start_pos > 0) {
        int bytes_to_move = buffer_pos - start_pos;
        if (bytes_to_move > 0) {
            for (int i = 0; i < bytes_to_move; i++) {
                buffer[i] = buffer[start_pos + i];
            }
        }
        buffer_pos = bytes_to_move;
        buffer[buffer_pos] = '\0';
    }
    
    // Look for end of sentence (just look for LF to simplify)
    int end_pos = -1;
    for (int i = 0; i < buffer_pos; i++) {
        if (buffer[i] == '\n') {
            end_pos = i;
            break;
        }
    }
    
    if (end_pos == -1) {
        // No end marker found yet
        return 0;
    }
    
    // Calculate sentence length (including the end marker)
    int sentence_len = end_pos + 1;
    
    // Check if it fits in the destination
    if (sentence_len >= max_len) {
        // Remove this oversized sentence from the buffer
        if (end_pos + 1 < buffer_pos) {
            int bytes_to_move = buffer_pos - (end_pos + 1);
            for (int i = 0; i < bytes_to_move; i++) {
                buffer[i] = buffer[end_pos + 1 + i];
            }
            buffer_pos = bytes_to_move;
        } else {
            buffer_pos = 0;
        }
        buffer[buffer_pos] = '\0';
        return 0;
    }
    
    // Copy complete sentence to output
    for (int i = 0; i < sentence_len; i++) {
        sentence[i] = buffer[i];
    }
    sentence[sentence_len] = '\0';  // Ensure null termination
    
    // Remove processed data from buffer
    if (end_pos + 1 < buffer_pos) {
        int bytes_to_move = buffer_pos - (end_pos + 1);
        for (int i = 0; i < bytes_to_move; i++) {
            buffer[i] = buffer[end_pos + 1 + i];
        }
        buffer_pos = bytes_to_move;
    } else {
        buffer_pos = 0;
    }
    buffer[buffer_pos] = '\0';
    
    return 1;  // Successfully extracted sentence
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

GPSData* spin_gps() {
    char nmea_sentence[128];
    
    if (read_gps_data()) {
        while (extract_nmea_sentence(nmea_sentence, sizeof(nmea_sentence))) {
            // Process complete NMEA sentence
            //printf("%s", nmea_sentence);
            parse_nmea_sentence(&gps_data, nmea_sentence);
        }
    }
    return &gps_data;
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