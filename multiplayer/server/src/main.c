#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/select.h>

#include "../../shared/include/protocol.h"
#include "../../shared/include/game_types.h"
#include "../include/game.h"

#define SERVER_PORT 5000
#define BACKLOG 8
#define MAX_CLIENTS 2

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

static void serve_loop(int lfd) {
    int clients[MAX_CLIENTS]; memset(clients, -1, sizeof(clients));
    uint32_t pids[MAX_CLIENTS]; memset(pids, 0, sizeof(pids));
    server_game_t game; game_init(&game);

    for (;;) {
        // Build fd sets
        fd_set rfds; FD_ZERO(&rfds); FD_SET(lfd, &rfds);
        int maxfd = lfd;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i] >= 0) { FD_SET(clients[i], &rfds); if (clients[i] > maxfd) maxfd = clients[i]; }
        }
        struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 200000; // 200ms tick
        int r = select(maxfd+1, &rfds, NULL, NULL, &tv);
        if (r < 0) continue;

        // New connection
        if (FD_ISSET(lfd, &rfds)) {
            int cfd = accept(lfd, NULL, NULL);
            if (cfd >= 0) {
                int slot = -1; for (int i = 0; i < MAX_CLIENTS; ++i) if (clients[i] < 0) { slot = i; break; }
                if (slot >= 0) {
                    uint32_t pid = (uint32_t)(rand() & 0x7fffffff);
                    if (game_add_player(&game, pid) == 0) {
                        clients[slot] = cfd; pids[slot] = pid;
                        printf("[server] client connected (fd=%d) -> player %u (slot %d)\n", cfd, pid, slot);
                        msg_header_t hdr = { .version=PROTOCOL_VERSION, .type=MSG_WELCOME, .length=sizeof(msg_welcome_t), .seq=1, .tick=game.tick };
                        msg_welcome_t w = { .player_id = pid };
                        send_frame(cfd, &hdr, &w, sizeof(w));
                    } else {
                        close(cfd);
                    }
                } else {
                    close(cfd);
                }
            }
        }

        // Read inputs
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int fd = clients[i]; if (fd < 0) continue;
            if (!FD_ISSET(fd, &rfds)) continue;
            msg_header_t in_h = {0}; unsigned char ibuf[256]; uint16_t ilen=0;
            if (recv_frame(fd, &in_h, ibuf, sizeof(ibuf), &ilen) < 0) {
                printf("[server] client %u disconnected\n", pids[i]);
                game_remove_player(&game, pids[i]);
                close(fd); clients[i] = -1; pids[i] = 0; continue;
            }
            if (in_h.type == MSG_INPUT && ilen == sizeof(msg_input_t)) {
                msg_input_t in; memcpy(&in, ibuf, sizeof(in));
                game_apply_input(&game, pids[i], &in);
            }
        }

        // Update and broadcast snapshot
        game_update(&game);
        game_snapshot_t snap; game_build_snapshot(&game, &snap);
        msg_header_t sh = { .version=PROTOCOL_VERSION, .type=MSG_SNAPSHOT, .length=(uint16_t)sizeof(snap), .seq=game.tick, .tick=game.tick };
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i] >= 0) {
                if (send_frame(clients[i], &sh, &snap, (uint16_t)sizeof(snap)) < 0) {
                    printf("[server] send error -> disconnect player %u\n", pids[i]);
                    game_remove_player(&game, pids[i]);
                    close(clients[i]); clients[i] = -1; pids[i] = 0;
                }
            }
        }
    }
}
 

int main(void) {
    int lfd = listen_tcp(SERVER_PORT);
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("Server listening on :%d\n", SERVER_PORT);
    serve_loop(lfd);
    return 0;
}
