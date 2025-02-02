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

typedef struct message {
    message_type type;
    position pos;
    int id;
    int status;
} message;

typedef struct circular_buffer {
    message* array;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} circular_buffer;

void buffer_init(circular_buffer* buf, int capacity);
void buffer_destroy(circular_buffer* buf);
void buffer_put(circular_buffer* buf, message* msg);
void buffer_get(circular_buffer* buf, message* msg);


#endif // BUFFER_H