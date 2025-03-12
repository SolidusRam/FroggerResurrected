#ifndef BUFFER_H
#define BUFFER_H
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define BUFFER_SIZE 50        // Dimensione del buffer circolare 
typedef struct {
    char type;    // 'F' per frog (rana), 'C' per coccodrillo
    int x;        // Posizione X assoluta
    int y;        // Posizione Y assoluta
    int dx;       // Spostamento X (incremento/decremento)
    int dy;       // Spostamento Y (incremento/decremento)
    int width;    // Larghezza per la scomparsa
    int height;   // Altezza elemento
    int id;       // Identificatore dell'elemento
    bool gameOver; // Flag per terminare il gioco
    int round;    // Numero della manche corrente
} Position;

typedef struct {
    Position data[BUFFER_SIZE]; // Array circolare
    int in;                     // Indice di inserimento
    int out;                    // Indice di estrazione
    pthread_mutex_t mutex;      // Mutex per proteggere l'accesso al buffer
    sem_t empty;                // Semaforo per contare gli slot vuoti
    sem_t full;                 // Semaforo per contare gli slot pieni
} Buffer;

// Variabili globali - dichiarate come extern
extern Buffer buffer;
extern bool game_over;
extern int maxY, maxX;
extern pthread_mutex_t lock;
extern int guardY, guardX;

// Prototipi di funzioni
void initBuffer(Buffer *buffer);
void destroyBuffer(Buffer *buffer);
void produce(Buffer *buffer, Position position);
Position consume(Buffer *buffer);

#endif