#ifndef BUFFER_H
#define BUFFER_H

#include "game.h"

#include <pthread.h>
#define BUFFER_SIZE 100

typedef enum {
    MSG_PLAYER,
    MSG_CROCODILE,
    MSG_GAME
} message_type;

typedef struct {
    message_type type;
    struct position pos;
    int id;
    int status;
} message;

typedef struct {
    message* array;          // Changed from position* to message*
    int capacity;
    int head;               // Points to next item to read
    int tail;               // Points to next free slot
    int count;              // Current number of items
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} circular_buffer;

void buffer_init(circular_buffer* buf, int capacity);
void buffer_destroy(circular_buffer* buf);
void buffer_put(circular_buffer* buf, message* msg);    // Changed parameter type
void buffer_get(circular_buffer* buf, message* msg);    // Changed parameter type


#endif // BUFFER_H