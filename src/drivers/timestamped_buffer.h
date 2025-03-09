#ifndef TIMESTAMPED_BUFFER_H
#define TIMESTAMPED_BUFFER_H

#include <pthread.h>

// Define buffer size
#define BUFFER_SIZE 100

// Simple integer buffer structure
typedef struct {
    int buffer[BUFFER_SIZE];          // Array of integers
    int write_index;                  // Index where next item will be written
    int read_index;                   // Index where next item will be read
    int count;                        // Number of items in buffer
    pthread_mutex_t mutex;            // Mutex for thread safety
    pthread_cond_t data_available;    // Condition for signaling data availability
} IntegerBuffer;

// Buffer operations
void buffer_init(IntegerBuffer* buffer);
int buffer_add(IntegerBuffer* buffer, int value);
int buffer_get(IntegerBuffer* buffer, int* value);
int buffer_peek(IntegerBuffer* buffer, int* value, int offset);
void buffer_cleanup(IntegerBuffer* buffer);

#endif // SIMPLE_BUFFER_H