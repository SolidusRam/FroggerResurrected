#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>

#include "../../shared/include/protocol.h"
#include "../../shared/include/game_types.h"
#include "../include/game.h"

#define SERVER_PORT 5000
#define BACKLOG 8
#define TICK_MS 150
#define HEARTBEAT_EVERY_TICKS 7  // ~1s a TICK_MS=150ms
#define CLIENT_TIMEOUT_SEC 5
#define SEND_TIMEOUT_MS 300
#define RECV_TIMEOUT_MS 2000

typedef enum { SLOT_EMPTY = 0, SLOT_PENDING, SLOT_ACTIVE } slot_state_t;

typedef struct {
    int fd;
    uint32_t player_id;
    slot_state_t state;
    time_t last_seen;
} client_slot_t;

// Stato condiviso tra il thread di rete (accept/JOIN/input) e il thread di
// simulazione (tick fisso + broadcast). Protetto da un unico mutex: con al
// più MAX_PLAYERS client e invii limitati da SO_SNDTIMEO, il locking a grana
// grossa resta semplice da ragionare senza diventare un collo di bottiglia.
typedef struct {
    pthread_mutex_t lock;
    server_game_t game;
    client_slot_t clients[MAX_PLAYERS];
    int listen_fd;
} shared_state_t;

static volatile sig_atomic_t g_running = 1;

static void handle_shutdown_signal(int sig) {
    (void)sig;
    g_running = 0;
}

static void configure_client_socket(int fd) {
    struct timeval snd_tv = { .tv_sec = SEND_TIMEOUT_MS / 1000, .tv_usec = (SEND_TIMEOUT_MS % 1000) * 1000 };
    struct timeval rcv_tv = { .tv_sec = RECV_TIMEOUT_MS / 1000, .tv_usec = (RECV_TIMEOUT_MS % 1000) * 1000 };
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &snd_tv, sizeof(snd_tv));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &rcv_tv, sizeof(rcv_tv));
    int on = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
}

static int listen_tcp(uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }
    int on = 1; setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); exit(1); }
    if (listen(fd, BACKLOG) < 0) { perror("listen"); exit(1); }
    return fd;
}

// Richiede shared->lock già acquisito dal chiamante.
static void drop_client_locked(shared_state_t* shared, int i, const char* reason) {
    client_slot_t* c = &shared->clients[i];
    if (c->state == SLOT_EMPTY) return;
    if (c->state == SLOT_ACTIVE) {
        printf("[server] player %u disconnected (%s)\n", c->player_id, reason);
        game_remove_player(&shared->game, c->player_id);
    }
    close(c->fd);
    c->fd = -1;
    c->state = SLOT_EMPTY;
    c->player_id = 0;
}

// Thread di simulazione: avanza il gioco a tick fisso, invia heartbeat (PING)
// e snapshot, e disconnette i client silenziosi da troppo tempo. Un client
// bloccato può stallare al più SEND_TIMEOUT_MS grazie al timeout sul socket,
// invece di bloccare indefinitamente l'intero server come nella versione POC.
static void* simulation_thread(void* arg) {
    shared_state_t* shared = (shared_state_t*)arg;
    uint32_t tick_count = 0;
    while (g_running) {
        pthread_mutex_lock(&shared->lock);

        game_update(&shared->game);
        time_t now = time(NULL);
        bool send_ping = (tick_count % HEARTBEAT_EVERY_TICKS) == 0;

        for (int i = 0; i < MAX_PLAYERS; ++i) {
            client_slot_t* c = &shared->clients[i];
            if (c->state != SLOT_ACTIVE) continue;
            if (now - c->last_seen > CLIENT_TIMEOUT_SEC) {
                drop_client_locked(shared, i, "heartbeat timeout");
                continue;
            }
            if (send_ping) {
                msg_header_t ph = { .version=PROTOCOL_VERSION, .type=MSG_PING, .length=0, .seq=0, .tick=shared->game.tick };
                if (send_frame(c->fd, &ph, NULL, 0) < 0) { drop_client_locked(shared, i, "send error"); continue; }
            }
        }

        game_snapshot_t snap; game_build_snapshot(&shared->game, &snap);
        msg_header_t sh = { .version=PROTOCOL_VERSION, .type=MSG_SNAPSHOT, .length=(uint16_t)sizeof(snap), .seq=shared->game.tick, .tick=shared->game.tick };
        for (int i = 0; i < MAX_PLAYERS; ++i) {
            client_slot_t* c = &shared->clients[i];
            if (c->state != SLOT_ACTIVE) continue;
            if (send_frame(c->fd, &sh, &snap, (uint16_t)sizeof(snap)) < 0) {
                drop_client_locked(shared, i, "send error");
            }
        }

        pthread_mutex_unlock(&shared->lock);
        tick_count++;
        struct timespec ts = { .tv_sec = 0, .tv_nsec = (long)TICK_MS * 1000000L };
        nanosleep(&ts, NULL);
    }
    return NULL;
}

static int find_free_slot(shared_state_t* shared) {
    for (int i = 0; i < MAX_PLAYERS; ++i) if (shared->clients[i].state == SLOT_EMPTY) return i;
    return -1;
}

// Thread di rete (main): accept, handshake JOIN->WELCOME, letture di input/PONG.
static void network_loop(shared_state_t* shared) {
    for (;;) {
        pthread_mutex_lock(&shared->lock);
        if (!g_running) { pthread_mutex_unlock(&shared->lock); break; }

        fd_set rfds; FD_ZERO(&rfds); FD_SET(shared->listen_fd, &rfds);
        int maxfd = shared->listen_fd;
        for (int i = 0; i < MAX_PLAYERS; ++i) {
            int fd = shared->clients[i].fd;
            if (fd >= 0) { FD_SET(fd, &rfds); if (fd > maxfd) maxfd = fd; }
        }
        pthread_mutex_unlock(&shared->lock);

        struct timeval tv = { .tv_sec = 0, .tv_usec = 200000 };
        int r = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (r < 0) { if (errno == EINTR) continue; break; }
        if (r == 0) continue;

        pthread_mutex_lock(&shared->lock);

        if (FD_ISSET(shared->listen_fd, &rfds)) {
            int cfd = accept(shared->listen_fd, NULL, NULL);
            if (cfd >= 0) {
                int slot = find_free_slot(shared);
                if (slot < 0) {
                    close(cfd);
                } else {
                    configure_client_socket(cfd);
                    shared->clients[slot] = (client_slot_t){ .fd = cfd, .player_id = 0, .state = SLOT_PENDING, .last_seen = time(NULL) };
                    printf("[server] client connected (fd=%d, slot=%d), waiting for JOIN\n", cfd, slot);
                }
            }
        }

        for (int i = 0; i < MAX_PLAYERS; ++i) {
            client_slot_t* c = &shared->clients[i];
            if (c->state == SLOT_EMPTY || c->fd < 0 || !FD_ISSET(c->fd, &rfds)) continue;

            msg_header_t h = {0}; unsigned char ibuf[256]; uint16_t ilen = 0;
            if (recv_frame(c->fd, &h, ibuf, sizeof(ibuf), &ilen) < 0) {
                drop_client_locked(shared, i, "connection closed");
                continue;
            }
            c->last_seen = time(NULL);

            if (c->state == SLOT_PENDING) {
                if (h.type != MSG_JOIN) continue;
                uint32_t pid = (uint32_t)(rand() & 0x7fffffff);
                if (game_add_player(&shared->game, pid) != 0) {
                    drop_client_locked(shared, i, "server full");
                    continue;
                }
                c->player_id = pid;
                c->state = SLOT_ACTIVE;
                msg_header_t wh = { .version=PROTOCOL_VERSION, .type=MSG_WELCOME, .length=sizeof(msg_welcome_t), .seq=1, .tick=shared->game.tick };
                msg_welcome_t w = { .player_id = pid };
                if (send_frame(c->fd, &wh, &w, sizeof(w)) < 0) { drop_client_locked(shared, i, "send error"); continue; }
                printf("[server] player %u joined (slot %d)\n", pid, i);
                continue;
            }

            if (h.type == MSG_INPUT && ilen == sizeof(msg_input_t)) {
                msg_input_t in; memcpy(&in, ibuf, sizeof(in));
                game_apply_input(&shared->game, c->player_id, &in);
            }
            // MSG_PONG: last_seen è già stato aggiornato sopra, nessuna azione ulteriore.
        }

        pthread_mutex_unlock(&shared->lock);
    }
}

int main(void) {
    signal(SIGPIPE, SIG_IGN); // scrivere su un socket chiuso deve fallire con EPIPE, non uccidere il processo

    struct sigaction sa = {0};
    sa.sa_handler = handle_shutdown_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    shared_state_t shared;
    pthread_mutex_init(&shared.lock, NULL);
    game_init(&shared.game);
    for (int i = 0; i < MAX_PLAYERS; ++i) shared.clients[i] = (client_slot_t){ .fd = -1, .player_id = 0, .state = SLOT_EMPTY, .last_seen = 0 };
    shared.listen_fd = listen_tcp(SERVER_PORT);

    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("Server listening on :%d (max %d players)\n", SERVER_PORT, MAX_PLAYERS);

    pthread_t sim_tid;
    if (pthread_create(&sim_tid, NULL, simulation_thread, &shared) != 0) {
        perror("pthread_create"); exit(1);
    }

    network_loop(&shared);

    printf("[server] shutting down...\n");
    pthread_join(sim_tid, NULL);

    pthread_mutex_lock(&shared.lock);
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (shared.clients[i].fd >= 0) close(shared.clients[i].fd);
    }
    pthread_mutex_unlock(&shared.lock);
    close(shared.listen_fd);
    pthread_mutex_destroy(&shared.lock);
    return 0;
}
