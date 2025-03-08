// shared_buffer.c
#include "timestamped_buffer.h"
#include <stdlib.h>
#include <string.h>

void buffer_init(SensorBuffer* buffer) {
    buffer->write_index = 0;
    buffer->read_index = 0;
    buffer->count = 0;
    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->data_available, NULL);
}

int buffer_add(SensorBuffer* buffer, SensorPacket* data) {
    int ret = 0;
    
    pthread_mutex_lock(&buffer->mutex);
    
    if (buffer->count < BUFFER_SIZE) {
        // Copy data to the buffer
        memcpy(&buffer->buffer[buffer->write_index], data, sizeof(SensorPacket));
        
        // Update write_index and count
        buffer->write_index = (buffer->write_index + 1) % BUFFER_SIZE;
        buffer->count++;
        
        // Signal that data is available
        pthread_cond_signal(&buffer->data_available);
        
        ret = 1;
    } else {
        // Buffer is full
        ret = 0;
    }
    
    pthread_mutex_unlock(&buffer->mutex);
    
    return ret;
}

int buffer_get(SensorBuffer* buffer, SensorPacket* data) {
    int ret = 0;
    
    pthread_mutex_lock(&buffer->mutex);
    
    while (buffer->count == 0) {
        // Wait for data to become available
        pthread_cond_wait(&buffer->data_available, &buffer->mutex);
    }
    
    // Copy data from the buffer
    memcpy(data, &buffer->buffer[buffer->read_index], sizeof(SensorPacket));
    
    // Update read_index and count
    buffer->read_index = (buffer->read_index + 1) % BUFFER_SIZE;
    buffer->count--;
    
    pthread_mutex_unlock(&buffer->mutex);
    
    return 1;
}

void buffer_cleanup(SensorBuffer* buffer) {
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->data_available);
}