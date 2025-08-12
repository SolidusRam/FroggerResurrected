#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>

#include "../../shared/include/protocol.h"
#include "../../shared/include/game_types.h"

static int connect_tcp(const char* host, uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }
    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) { perror("inet_pton"); exit(1); }
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("connect"); exit(1); }
    return fd;
}

int main(int argc, char** argv) {
    const char* host = (argc > 1 ? argv[1] : "127.0.0.1");
    uint16_t port = (argc > 2 ? (uint16_t)atoi(argv[2]) : 5000);
    int fd = connect_tcp(host, port);

    // Attendi WELCOME
    msg_header_t hdr; unsigned char buf[2048]; uint16_t len=0;
    if (recv_frame(fd, &hdr, buf, sizeof(buf), &len) < 0 || hdr.type != MSG_WELCOME) {
        fprintf(stderr, "Handshake failed\n"); close(fd); return 1;
    }
    msg_welcome_t w = {0};
    if (len == sizeof(w)) memcpy(&w, buf, len);
    printf("Connected. PlayerId=%u\n", w.player_id);

    // Loop: invia input da tastiera (WASD) e ricevi snapshot
    printf("Controls: WASD + Enter (didattico: leggo da stdin riga intera)\n");
    for (;;) {
        // 1) Se c'è input da stdin, leggi una riga e manda MSG_INPUT
        fd_set rfds; FD_ZERO(&rfds); FD_SET(0, &rfds); FD_SET(fd, &rfds);
        int maxfd = fd;
        struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 200000; // 200ms timeout
        int r = select(maxfd+1, &rfds, NULL, NULL, &tv);
        if (r < 0) break;

        if (FD_ISSET(0, &rfds)) {
            char line[64];
            if (!fgets(line, sizeof(line), stdin)) { /* EOF */ }
            unsigned buttons = 0;
            for (char* p=line; *p; ++p) {
                if (*p=='a' || *p=='A') buttons |= IN_LEFT;
                if (*p=='d' || *p=='D') buttons |= IN_RIGHT;
                if (*p=='w' || *p=='W') buttons |= IN_UP;
                if (*p=='s' || *p=='S') buttons |= IN_DOWN;
            }
            if (buttons) {
                msg_input_t in = { .client_seq = 1, .buttons = buttons };
                msg_header_t ih = { .version=PROTOCOL_VERSION, .type=MSG_INPUT, .length=(uint16_t)sizeof(in), .seq=1, .tick=0 };
                send_frame(fd, &ih, &in, (uint16_t)sizeof(in));
            }
        }

        // // 2) Se c'è dato sul socket, ricevi e stampa snapshot
        // if (FD_ISSET(fd, &rfds)) {
        //     uint16_t plen = 0;
        //     if (recv_frame(fd, &hdr, buf, sizeof(buf), &plen) < 0) break;
        //     if (hdr.type == MSG_SNAPSHOT && plen == sizeof(game_snapshot_t)) {
        //         game_snapshot_t snap;
        //         memcpy(&snap, buf, sizeof(snap));
        //         int x = snap.players[0].ent.box.x;
        //         int y = snap.players[0].ent.box.y;
        //         printf("SNAPSHOT tick=%u player0=(%d,%d) score=%d\n", hdr.tick, x, y, snap.players[0].score);
        //     }
        // }
    }
    printf("Disconnected.\n");
    close(fd);
    return 0;
}
