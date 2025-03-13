#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <ncurses.h>
#include <time.h>
#include <pthread.h>

// Game constants
#define GAME_WIDTH 80
#define GAME_HEIGHT 24
#define FLOOR_HEIGHT 20
#define LANES 8
#define LANE_HEIGHT 2
#define MAX_CROCODILES 16
#define MAX_BULLETS 100
#define NUM_TANE 5
#define TANA_WIDTH 7
#define TANA_HEIGHT 1

// Shared game state
typedef struct {
    char c;
    int x;
    int y;
    int width;
    int height;
    int id;
    bool active;
    bool collision;
    pthread_mutex_t mutex;  // Mutex for this position
} position;

// Den structure
typedef struct {
    int x;
    int y;
    bool occupata;
} tana;

// Structure for bullets
typedef struct {
    position pos;
    int direction;          // 1 for right, -1 for left
    pthread_t thread_id;    // Thread ID for this bullet
    bool is_enemy;          // true if from crocodile, false if from player
} bullet;

// Game state structure - shared between all threads
typedef struct {
    // Player state
    position player;
    int vite;
    int score;
    
    // Game objects
    position crocodiles[MAX_CROCODILES];
    bullet bullets[MAX_BULLETS];
    tana tane[NUM_TANE];
    
    // Game status
    bool game_over;
    int remaining_time;
    int max_time;
    time_t last_update;
    
    // Synchronization
    pthread_mutex_t game_mutex;       // Overall game state mutex
    pthread_mutex_t screen_mutex;     // Screen drawing mutex
    pthread_cond_t game_update_cond;  // Condition for game updates
    
    // Den status
    int tane_occupate;
} game_state;

// Thread argument structures
typedef struct {
    game_state* state;
    int id;
} crocodile_args;

typedef struct {
    game_state* state;
    int bullet_id;
} bullet_args;

// Thread function declarations
void* player_thread(void* arg);
void* crocodile_thread(void* arg);
void* bullet_thread(void* arg);
void* game_thread(void* arg);

// Game utility functions
bool rana_coccodrillo(position* rana_pos, position crocodile_positions[], int num_coccodrilli, int* direction);
bool frog_on_the_water(position* rana_pos);
bool check_den_collision(position* rana_pos, tana* tane, int num_tane);
int find_free_bullet_slot(game_state* state);

// Initialize positions
void init_game_state(game_state* state);

extern char rana_sprite[2][5];

#endif // GAME_H