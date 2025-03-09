#include "timestamped_buffer.h"
#include <string.h>
#include <stdio.h>

void buffer_init(IntegerBuffer* buffer) {
    buffer->write_index = 0;
    buffer->read_index = 0;
    buffer->count = 0;
    
    // Initialize mutex with process-shared attribute
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&buffer->mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    
    // Initialize condition variable with process-shared attribute
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&buffer->data_available, &cond_attr);
    pthread_condattr_destroy(&cond_attr);
    
    printf("Buffer initialized: write_index=%d, read_index=%d, count=%d\n", 
           buffer->write_index, buffer->read_index, buffer->count);
}

int buffer_add(IntegerBuffer* buffer, int value) {
    pthread_mutex_lock(&buffer->mutex);
    
    if (buffer->count < BUFFER_SIZE) {
        // Add the integer to the buffer
        buffer->buffer[buffer->write_index] = value;
        
        // Update write_index and count
        buffer->write_index = (buffer->write_index + 1) % BUFFER_SIZE;
        buffer->count++;
        
        printf("Added value %d, write_index=%d, read_index=%d, count=%d\n", 
               value, buffer->write_index, buffer->read_index, buffer->count);
        
        // Signal that data is available
        pthread_cond_signal(&buffer->data_available);
        
        pthread_mutex_unlock(&buffer->mutex);
        return 1;  // Success
    }
    
    pthread_mutex_unlock(&buffer->mutex);
    printf("Buffer full, can't add value %d\n", value);
    return 0;  // Buffer full
}

int buffer_get(IntegerBuffer* buffer, int* value) {
    pthread_mutex_lock(&buffer->mutex);
    
    while (buffer->count == 0) {
        // Wait for data to become available
        printf("Waiting for data...\n");
        pthread_cond_wait(&buffer->data_available, &buffer->mutex);
    }
    
    // Get the integer from the buffer
    *value = buffer->buffer[buffer->read_index];
    
    // Update read_index and count
    buffer->read_index = (buffer->read_index + 1) % BUFFER_SIZE;
    buffer->count--;
    
    printf("Read value %d, write_index=%d, read_index=%d, count=%d\n", 
           *value, buffer->write_index, buffer->read_index, buffer->count);
    
    pthread_mutex_unlock(&buffer->mutex);
    return 1;  // Success
}

int buffer_peek(IntegerBuffer* buffer, int* value, int offset) {
    pthread_mutex_lock(&buffer->mutex);
    
    if (offset >= buffer->count) {
        pthread_mutex_unlock(&buffer->mutex);
        return 0;  // Not enough data
    }
    
    int index = (buffer->read_index + offset) % BUFFER_SIZE;
    *value = buffer->buffer[index];
    
    printf("Peeked value %d at offset %d\n", *value, offset);
    
    pthread_mutex_unlock(&buffer->mutex);
    return 1;  // Success
}

void buffer_cleanup(IntegerBuffer* buffer) {
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->data_available);
    printf("Buffer cleaned up\n");
}