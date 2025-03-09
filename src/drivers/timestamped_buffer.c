// simple_buffer.c
#include "timestamped_buffer.h"
#include <string.h>

void buffer_init(SensorBuffer* buffer) {
    buffer->write_index = 0;
    buffer->read_index = 0;
    buffer->count = 0;
    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->data_available, NULL);
}

int buffer_add(SensorBuffer* buffer, SensorData* data) {
    pthread_mutex_lock(&buffer->mutex);
    
    if (buffer->count < BUFFER_SIZE) {
        // Copy data to the buffer
        memcpy(&buffer->buffer[buffer->write_index], data, sizeof(SensorData));
        
        // Update write_index and count
        buffer->write_index = (buffer->write_index + 1) % BUFFER_SIZE;
        buffer->count++;
        
        // Signal that data is available
        pthread_cond_signal(&buffer->data_available);
        
        pthread_mutex_unlock(&buffer->mutex);
        return 1;  // Success
    }
    
    pthread_mutex_unlock(&buffer->mutex);
    return 0;  // Buffer full
}

int buffer_get(SensorBuffer* buffer, SensorData* data) {
    pthread_mutex_lock(&buffer->mutex);
    
    while (buffer->count == 0) {
        // Wait for data to become available
        pthread_cond_wait(&buffer->data_available, &buffer->mutex);
    }
    
    // Copy data from the buffer
    memcpy(data, &buffer->buffer[buffer->read_index], sizeof(SensorData));
    
    // Update read_index and count
    buffer->read_index = (buffer->read_index + 1) % BUFFER_SIZE;
    buffer->count--;
    
    pthread_mutex_unlock(&buffer->mutex);
    return 1;  // Success
}

// Get data at an offset from read_index without removing it
int buffer_peek(SensorBuffer* buffer, SensorData* data, int offset) {
    pthread_mutex_lock(&buffer->mutex);
    
    if (offset >= buffer->count) {
        pthread_mutex_unlock(&buffer->mutex);
        return 0;  // Not enough data
    }
    
    int index = (buffer->read_index + offset) % BUFFER_SIZE;
    memcpy(data, &buffer->buffer[index], sizeof(SensorData));
    
    pthread_mutex_unlock(&buffer->mutex);
    return 1;  // Success
}

void buffer_cleanup(SensorBuffer* buffer) {
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->data_available);
}