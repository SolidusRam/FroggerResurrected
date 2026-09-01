#include "../include/scoreboard.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>

#define SCOREBOARD_SCAN_CAP 4096

// flock() può essere interrotta da un segnale: va ritentata, non trattata come errore fatale.
static int flock_retry(int fd, int op) {
    int r;
    do { r = flock(fd, op); } while (r < 0 && errno == EINTR);
    return r;
}

int scoreboard_record(uint32_t player_id, int32_t score) {
    int fd = open(SCOREBOARD_PATH, O_CREAT | O_RDWR, 0644);
    if (fd < 0) return -1;

    if (flock_retry(fd, LOCK_EX) < 0) { close(fd); return -1; }

    if (lseek(fd, 0, SEEK_END) < 0) { flock_retry(fd, LOCK_UN); close(fd); return -1; }

    char line[128];
    int n = snprintf(line, sizeof(line), "%u %d %ld\n",
                      (unsigned)player_id, (int)score, (long)time(NULL));
    if (n < 0 || write(fd, line, (size_t)n) != n) {
        flock_retry(fd, LOCK_UN); close(fd); return -1;
    }

    flock_retry(fd, LOCK_UN);
    close(fd);
    return 0;
}

static int cmp_entry_desc(const void* a, const void* b) {
    const scoreboard_entry_t* ea = a;
    const scoreboard_entry_t* eb = b;
    if (ea->score != eb->score) return (eb->score - ea->score);
    return (int)(eb->timestamp - ea->timestamp);
}

int scoreboard_get_top(scoreboard_entry_t* out, int max_n) {
    if (!out || max_n <= 0) return 0;

    int fd = open(SCOREBOARD_PATH, O_CREAT | O_RDONLY, 0644);
    if (fd < 0) return 0;

    if (flock_retry(fd, LOCK_SH) < 0) { close(fd); return 0; }

    FILE* f = fdopen(fd, "r");
    if (!f) { flock_retry(fd, LOCK_UN); close(fd); return 0; }

    scoreboard_entry_t all[SCOREBOARD_SCAN_CAP];
    int count = 0;
    char line[128];
    while (count < SCOREBOARD_SCAN_CAP && fgets(line, sizeof(line), f)) {
        unsigned pid; int score; long ts;
        if (sscanf(line, "%u %d %ld", &pid, &score, &ts) == 3) {
            all[count].player_id = pid;
            all[count].score = score;
            all[count].timestamp = ts;
            count++;
        }
        // righe malformate vengono semplicemente ignorate
    }

    fclose(f); // chiude anche fd; il lock viene rilasciato alla chiusura

    qsort(all, (size_t)count, sizeof(all[0]), cmp_entry_desc);

    int n = (count < max_n) ? count : max_n;
    for (int i = 0; i < n; ++i) out[i] = all[i];
    return n;
}
