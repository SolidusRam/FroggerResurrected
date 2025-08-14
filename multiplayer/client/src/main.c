// Client minimalista con ncurses: input raw non-bloccante + render testuale
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <ncurses.h>
#include "../include/ui.h"

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
    msg_header_t hdr; unsigned char buf[8192]; uint16_t len=0;
    if (recv_frame(fd, &hdr, buf, sizeof(buf), &len) < 0 || hdr.type != MSG_WELCOME) {
        fprintf(stderr, "Handshake failed\n"); close(fd); return 1;
    }
    msg_welcome_t w = {0};
    if (len == sizeof(w)) memcpy(&w, buf, len);
    uint32_t local_player_id = w.player_id;
    ui_init();

    // Stato locale per il render (ultimo snapshot)
    int px = 0, py = 0, score = 0; unsigned tick = 0;
    // snapshot buffer (ultimo) per disegno di coccodrilli e tane
    game_snapshot_t last = {0};

    // Loop: invia input da tastiera (WASD/frecce) e ricevi snapshot
    for (;;) {
        // 1) Poll input locale (getch non-bloccante); invia un INPUT per tasto
        for (;;) {
            int ch = getch();
            if (ch == ERR) break; // nessun altro tasto
            unsigned buttons = 0;
            if (ch == 'q' || ch == 'Q') { endwin(); close(fd); return 0; }
            switch (ch) {
                case 'a': case 'A': case KEY_LEFT:  buttons |= IN_LEFT;  break;
                case 'd': case 'D': case KEY_RIGHT: buttons |= IN_RIGHT; break;
                case 'w': case 'W': case KEY_UP:    buttons |= IN_UP;    break;
                case 's': case 'S': case KEY_DOWN:  buttons |= IN_DOWN;  break;
                default: break;
            }
            if (buttons) {
                msg_input_t in = { .client_seq = 0, .buttons = buttons };
                msg_header_t ih = { .version=PROTOCOL_VERSION, .type=MSG_INPUT, .length=(uint16_t)sizeof(in), .seq=0, .tick=0 };
                send_frame(fd, &ih, &in, (uint16_t)sizeof(in));
            }
        }

        // 2) Attendi rete per un breve intervallo, così non busy-waitiamo
        fd_set rfds; FD_ZERO(&rfds); FD_SET(fd, &rfds);
        struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 50000; // 50ms
        int r = select(fd+1, &rfds, NULL, NULL, &tv);
        if (r < 0) { break; }
        if (r > 0 && FD_ISSET(fd, &rfds)) {
            uint16_t plen = 0;
            if (recv_frame(fd, &hdr, buf, sizeof(buf), &plen) < 0) break;
            if (hdr.type == MSG_SNAPSHOT && plen == sizeof(game_snapshot_t)) {
                memcpy(&last, buf, sizeof(last));
                // best-effort: set px/py to our player if present
                for (int i = 0; i < last.num_players; ++i) {
                    if ((uint32_t)last.players[i].id == local_player_id) {
                        px = last.players[i].ent.box.x;
                        py = last.players[i].ent.box.y;
                        score = last.players[i].score;
                        break;
                    }
                }
                tick = hdr.tick;
            }
        }

        // 3) Render snapshot completo
    ui_draw_snapshot(&last, local_player_id);
    }
    ui_teardown();
    printf("Disconnected.\n");
    close(fd);
    return 0;
}
