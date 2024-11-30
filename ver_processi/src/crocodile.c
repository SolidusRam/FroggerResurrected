#include "../include/crocodile.h"
#include <time.h>

void coccodrillo(int pipeout) {
    const int NUM_CROCODILES = 8;
    struct position crocodiles[NUM_CROCODILES];
    int speeds[NUM_CROCODILES];     // Different speeds for each lane
    int active[NUM_CROCODILES];     // Track if crocodile is active
    int fade_states[NUM_CROCODILES]; // Track fading animation (0-5)
    
    srand(time(NULL));
    
    // Initialize crocodiles with different speeds and start times
    for (int i = 0; i < NUM_CROCODILES; i++) {
        crocodiles[i].c = 'C';
        crocodiles[i].width = 5;
        crocodiles[i].height = 1;
        crocodiles[i].x = (i % 2 == 0) ? 1 : GAME_WIDTH - 6;
        crocodiles[i].y = 4 + (i * LANE_HEIGHT);
        
        speeds[i] = 50000 + (rand() % 100000);  // Random speed between 50-150ms
        active[i] = rand() % 2;                 // Random initial active state
        fade_states[i] = 0;                     // No fading initially
    }

    while (1) {
        for (int i = 0; i < NUM_CROCODILES; i++) {
            if (!active[i]) {
                if (rand() % 100 < 40) { // 40% chance to activate each iteration
                    active[i] = 1;
                }
                continue;
            }

            if (fade_states[i] > 0) {
                // Handle fading animation
                crocodiles[i].width = fade_states[i];
                fade_states[i]--;
                if (fade_states[i] == 0) {
                    // Reset position after fade
                    crocodiles[i].x = (i % 2 == 0) ? 1 : GAME_WIDTH - 6;
                    crocodiles[i].width = 5;
                }
            } else {
                // Normal movement
                if (i % 2 == 0) {
                    crocodiles[i].x += 1;
                    if (crocodiles[i].x > GAME_WIDTH - 6) {
                        fade_states[i] = 5; // Start fade out
                    }
                } else {
                    crocodiles[i].x -= 1;
                    if (crocodiles[i].x < 1) {
                        fade_states[i] = 5; // Start fade out
                    }
                }
            }
            
            write(pipeout, &crocodiles[i], sizeof(struct position));
            usleep(speeds[i]); // Individual speed per lane
        }
    }
}