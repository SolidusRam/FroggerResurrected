#include "../include/game.h"
#include <stdlib.h>

void buffer_init(circular_buffer* buf, int capacity) {
    buf->array = malloc(capacity * sizeof(game_message));
    buf->capacity = capacity;
    buf->head = 0;
    buf->tail = 0;
    buf->count = 0;
    pthread_mutex_init(&buf->mutex, NULL);
    pthread_cond_init(&buf->not_full, NULL);
    pthread_cond_init(&buf->not_empty, NULL);
}

void buffer_destroy(circular_buffer* buf) {
    free(buf->array);
    pthread_mutex_destroy(&buf->mutex);
    pthread_cond_destroy(&buf->not_full);
    pthread_cond_destroy(&buf->not_empty);
}

void buffer_put(circular_buffer* buf, game_message* item) {
    pthread_mutex_lock(&buf->mutex);
    
    // Wait while buffer is full
    while (buf->count == buf->capacity) {
        pthread_cond_wait(&buf->not_full, &buf->mutex);
    }
    
    // Add item to buffer
    buf->array[buf->tail] = *item;
    buf->tail = (buf->tail + 1) % buf->capacity;
    buf->count++;
    
    pthread_cond_signal(&buf->not_empty);
    pthread_mutex_unlock(&buf->mutex);
}

void buffer_get(circular_buffer* buf, game_message* msg) {
    pthread_mutex_lock(&buf->mutex);
    
    // Wait if buffer is empty
    while (buf->count == 0) {
        pthread_cond_wait(&buf->not_empty, &buf->mutex);
    }
    
    // Get item from buffer
    *msg = buf->array[buf->head];
    buf->head = (buf->head + 1) % buf->capacity;
    buf->count--;
    
    pthread_cond_signal(&buf->not_full);
    pthread_mutex_unlock(&buf->mutex);
}

bool buffer_try_get(circular_buffer* buf, game_message* msg) {
    pthread_mutex_lock(&buf->mutex);
    
    if (buf->count == 0) {
        pthread_mutex_unlock(&buf->mutex);
        return false;
    }
    
    // Get item from buffer
    *msg = buf->array[buf->head];
    buf->head = (buf->head + 1) % buf->capacity;
    buf->count--;
    
    pthread_cond_signal(&buf->not_full);
    pthread_mutex_unlock(&buf->mutex);
    return true;
}