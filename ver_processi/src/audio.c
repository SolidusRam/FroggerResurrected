#include "../include/audio.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h> // Added for waitpid

static pid_t bg_music_pid = -1; // PID of the shell process running the music loop

void play_sound(SoundType sound) {
    const char* sound_files[] = {
        "background.wav",     // SOUND_BACKGROUND (not used directly by this function)
        "blip.wav",           // SOUND_SHOOT
        "winning.wav",        // SOUND_TANA_ENTER
        "fall.wav",           // SOUND_SPLASH (for frog falling/losing life)
        "collision.wav",      // SOUND_COLLISION (can be used for other types of collisions)
        "game_over.wav",      // SOUND_GAME_OVER
        "victory.wav"         // SOUND_VICTORY
    };
    
    if (sound > SOUND_BACKGROUND && sound < (sizeof(sound_files) / sizeof(sound_files[0]))) {
        char command[300];
        snprintf(command, sizeof(command), "aplay ../audio/%s 2>/dev/null &", sound_files[sound]);
        system(command);
    }
}

void toggle_background_music(bool play) {
    if (play) {
        if (bg_music_pid == -1) { // Start only if not already playing
            bg_music_pid = fork();
            if (bg_music_pid == 0) { // Child process
                // Child becomes the shell running the loop.
                // Redirect stdout/stderr to /dev/null to prevent clutter.
                freopen("/dev/null", "w", stdout);
                freopen("/dev/null", "w", stderr);
                
                char* const argv_shell[] = {"sh", "-c", "while true; do aplay ../audio/background.wav; sleep 0.1; done", NULL};
                execvp("sh", argv_shell);
                
                // execvp only returns on error
                perror("execvp for background music shell failed");
                _exit(127); 
            } else if (bg_music_pid < 0) {
                perror("fork for background music failed");
                bg_music_pid = -1; 
            }
        }
    } else { // Stop music
        if (bg_music_pid > 0) {
            kill(bg_music_pid, SIGKILL); 
            waitpid(bg_music_pid, NULL, 0); 
            bg_music_pid = -1;
        }
    }
}

void cleanup_audio(void) {
    if (bg_music_pid > 0) {
        kill(bg_music_pid, SIGKILL);
        waitpid(bg_music_pid, NULL, 0);
        bg_music_pid = -1;
    }
    // Optional: attempt to kill any stray aplay processes.
    // Use with caution as it's a broad command.
    // system("killall -q aplay"); 
}