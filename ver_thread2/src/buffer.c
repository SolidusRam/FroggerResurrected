#include "../include/buffer.h"

// Definizioni delle variabili globali
Buffer buffer;
bool game_over = false;
int maxY, maxX;
pthread_mutex_t lock;
int guardY, guardX;

// Inizializzazione del buffer
void initBuffer(Buffer *buffer) {
    buffer->in = 0;
    buffer->out = 0;
    pthread_mutex_init(&buffer->mutex, NULL);
    sem_init(&buffer->empty, 0, BUFFER_SIZE); // Buffer inizialmente vuoto
    sem_init(&buffer->full, 0, 0);            // Nessun elemento da consumare
}

// Distruzione del buffer
void destroyBuffer(Buffer *buffer) {
    pthread_mutex_destroy(&buffer->mutex);
    sem_destroy(&buffer->empty);
    sem_destroy(&buffer->full);
}

// Inserimento nel buffer (operazione di produzione)
void produce(Buffer *buffer, Position position) {
    sem_wait(&buffer->empty); // Attende uno slot vuoto
    pthread_mutex_lock(&buffer->mutex);
    
    // Inserisce la posizione nel buffer
    buffer->data[buffer->in] = position;
    buffer->in = (buffer->in + 1) % BUFFER_SIZE; // Incremento circolare
    
    pthread_mutex_unlock(&buffer->mutex);
    sem_post(&buffer->full); // Segnala un nuovo elemento disponibile
}

// Estrazione dal buffer (operazione di consumo)
Position consume(Buffer *buffer) {
    Position position;
    
    sem_wait(&buffer->full); // Attende un elemento disponibile
    pthread_mutex_lock(&buffer->mutex);
    
    // Estrae la posizione dal buffer
    position = buffer->data[buffer->out];
    buffer->out = (buffer->out + 1) % BUFFER_SIZE; // Incremento circolare
    
    pthread_mutex_unlock(&buffer->mutex);
    sem_post(&buffer->empty); // Segnala uno slot libero
    
    return position;
}

