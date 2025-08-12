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

#define SERVER_PORT 5000
#define BACKLOG 8

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

static void handle_client(int cfd, uint32_t player_id) {
    // Handshake: invia WELCOME
    msg_header_t hdr = { .version=PROTOCOL_VERSION, .type=MSG_WELCOME, .length=sizeof(msg_welcome_t), .seq=1, .tick=0 };
    msg_welcome_t w = { .player_id = player_id };
    if (send_frame(cfd, &hdr, &w, sizeof(w)) < 0) { 
        perror("send WELCOME");
        close(cfd); 
        return; 
    }
    printf("[server] WELCOME sent to player %u\n", player_id);

    // Dimostrazione didattica: piccolo loop a 5 Hz che invia snapshot
    int px = 0, py = 10; // posizione giocatore controllata dagli INPUT del client
    for (uint32_t tick = 0; tick < 1000; ++tick) {
        // 1) Leggi eventuali INPUT dal client senza bloccare (select con timeout 0)
        fd_set rfds; FD_ZERO(&rfds); FD_SET(cfd, &rfds);
        struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 0;
        int r = select(cfd+1, &rfds, NULL, NULL, &tv);
        if (r > 0 && FD_ISSET(cfd, &rfds)) {
            msg_header_t in_h = {0};
            unsigned char ibuf[256];
            uint16_t ilen = 0;
            if (recv_frame(cfd, &in_h, ibuf, sizeof(ibuf), &ilen) < 0) {
                printf("[server] client %u disconnected or read error\n", player_id);
                break;
            }
            if (in_h.type == MSG_INPUT && ilen == sizeof(msg_input_t)) {
                msg_input_t in = {0};
                memcpy(&in, ibuf, sizeof(in));
                int dx = 0, dy = 0;
                if (in.buttons & IN_LEFT)  dx -= 1;
                if (in.buttons & IN_RIGHT) dx += 1;
                if (in.buttons & IN_UP)    dy -= 1;
                if (in.buttons & IN_DOWN)  dy += 1;
                px += dx; py += dy;
                if (px < 0) px = 0; if (px >= GAME_WIDTH) px = GAME_WIDTH-1;
                if (py < 0) py = 0; if (py >= GAME_HEIGHT) py = GAME_HEIGHT-1;
                printf("[server] INPUT from player %u buttons=%u -> pos=(%d,%d)\n", player_id, (unsigned)in.buttons, px, py);
            }
        }

        // 2) Prepara e invia lo SNAPSHOT per il tick corrente
        game_snapshot_t snap = {0};
        snap.tick = tick;
        snap.num_players = 1;
        snap.players[0].id = (int32_t)player_id;
        snap.players[0].connected = true;
        snap.players[0].lives = 3;
        snap.players[0].score = (int32_t)(tick * 10);
        snap.players[0].ent.id = (int32_t)player_id;
        snap.players[0].ent.active = true;
        snap.players[0].ent.box.w = 1;
        snap.players[0].ent.box.h = 1;
        snap.players[0].ent.box.x = px;
        snap.players[0].ent.box.y = py; // riga controllata dagli input

        msg_header_t sh = { .version=PROTOCOL_VERSION, .type=MSG_SNAPSHOT, .length=(uint16_t)sizeof(snap), .seq=tick+1, .tick=tick };
        if (send_frame(cfd, &sh, &snap, (uint16_t)sizeof(snap)) < 0) {
            perror("send SNAPSHOT");
            break;
        }
        printf("[server] SNAPSHOT tick=%u sent (x=%d,y=%d)\n", tick, snap.players[0].ent.box.x, snap.players[0].ent.box.y);
    struct timespec ts; ts.tv_sec = 0; ts.tv_nsec = 200000000; // 200ms
    nanosleep(&ts, NULL); // 5 Hz
    }
    printf("[server] closing connection with player %u\n", player_id);
    close(cfd);
}

int main(void) {
    int lfd = listen_tcp(SERVER_PORT);
    // assicura flush riga-per-riga in console
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("Server listening on :%d\n", SERVER_PORT);
    uint32_t next_id = 1;
    for (;;) {
        int cfd = accept(lfd, NULL, NULL);
        if (cfd < 0) continue;
        uint32_t pid = next_id++;
        printf("[server] client connected (fd=%d) -> player %u\n", cfd, pid);
        // Nota: per semplicità gestiamo un client alla volta; in seguito useremo thread o poll/epoll
        handle_client(cfd, pid);
    }
    return 0;
}
