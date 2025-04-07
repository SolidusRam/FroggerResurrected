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
#define BUFFER_SIZE 100

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

typedef enum {
    MSG_PLAYER,
    MSG_CROCODILE,
    MSG_BULLET
} message_type;

typedef struct {
    message_type type;
    position pos;
    int id;
    int direction;
    bool is_enemy;
} game_message;

typedef struct {
    game_message* array;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} circular_buffer;

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
    bool game_paused;       // Flag for game pause
    int remaining_time;
    int max_time;
    time_t last_update;
    
    // Synchronization
    pthread_mutex_t game_mutex;       // Overall game state mutex
    pthread_mutex_t screen_mutex;     // Screen drawing mutex
    pthread_cond_t game_update_cond;  // Condition for game updates
    
    // Den status
    int tane_occupate;
    
    // Player-crocodile association
    bool player_on_crocodile;         // Is player currently on a crocodile
    int player_crocodile_id;          // ID of crocodile player is riding

    circular_buffer event_buffer;
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
void* debug_bullet_test_thread(void* arg);  // Thread per test di debug

// Game utility functions
bool rana_coccodrillo(position* rana_pos, position crocodile_positions[], int num_coccodrilli, int* direction);
bool frog_on_the_water(position* rana_pos);
bool check_den_collision(position* rana_pos, tana* tane, int num_tane);
int find_free_bullet_slot(game_state* state);
void create_bullet(game_state* state, int x, int y, int direction, bool is_enemy);

// Initialize and cleanup functions
void init_game_state(game_state* state);
void destroy_game_state(game_state* state);
void update_player_on_crocodile(game_state* state);

extern char rana_sprite[2][5];

#endif // GAME_H