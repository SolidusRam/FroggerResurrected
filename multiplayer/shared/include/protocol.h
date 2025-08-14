// protocol.h — Protocollo di rete semplice per client/server
// Trasporto: TCP con framing length-prefixed: [uint16_be total_len][header][payload]

#pragma once
#include <stdint.h>

#define PROTOCOL_VERSION 1

typedef enum {
    MSG_JOIN = 1,       // C->S: richiesta ingresso (non usato nel POC)
    MSG_WELCOME = 2,    // S->C: playerId assegnato
    MSG_INPUT = 3,      // C->S: input corrente (non usato nel POC)
    MSG_SNAPSHOT = 4,   // S->C: stato del mondo (non usato nel POC)
    MSG_PING = 5,       // C->S
    MSG_PONG = 6        // S->C
} msg_type_t;

// Header che precede ogni payload
typedef struct {
    uint8_t  version;   // PROTOCOL_VERSION
    uint8_t  type;      // msg_type_t
    uint16_t length;    // payload bytes (informativo)
    uint32_t seq;       // numero sequenziale
    uint32_t tick;      // tick server (se rilevante)
} msg_header_t;

// INPUT payload: bitmask tasti + contatore locale
enum {
    IN_LEFT  = 1<<0,
    IN_RIGHT = 1<<1,
    IN_UP    = 1<<2,
    IN_DOWN  = 1<<3,
    IN_SHOOT = 1<<4
};

typedef struct {
    uint32_t client_seq;
    uint32_t buttons;   // bitmask IN_*
} msg_input_t;

typedef struct {
    uint32_t player_id;
} msg_welcome_t;

// API di framing (implementate in protocol.c)
int send_frame(int fd, const void* hdr, const void* payload, uint16_t payload_len);
int recv_frame(int fd, void* hdr_out, void* payload_buf, uint16_t buf_cap, uint16_t* out_len);
