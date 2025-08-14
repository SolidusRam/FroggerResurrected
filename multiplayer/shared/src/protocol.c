#include "../include/protocol.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static ssize_t writen_all(int fd, const void* vptr, size_t n) {
    const char* p = (const char*)vptr;
    size_t left = n;
    while (left > 0) {
        ssize_t w = write(fd, p, left);
        if (w < 0) { if (errno == EINTR) continue; return -1; }
        if (w == 0) return -1;
        left -= (size_t)w; p += w;
    }
    return (ssize_t)n;
}

static ssize_t readn_all(int fd, void* vptr, size_t n) {
    char* p = (char*)vptr;
    size_t left = n;
    while (left > 0) {
        ssize_t r = read(fd, p, left);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        if (r == 0) return -1;
        left -= (size_t)r; p += r;
    }
    return (ssize_t)n;
}

int send_frame(int fd, const void* hdr, const void* payload, uint16_t payload_len) {
    uint16_t total = (uint16_t)(sizeof(msg_header_t) + payload_len);
    uint16_t be_total = htons(total);
    if (writen_all(fd, &be_total, sizeof(be_total)) < 0) return -1;
    if (writen_all(fd, hdr, sizeof(msg_header_t)) < 0) return -1;
    if (payload_len && payload) {
        if (writen_all(fd, payload, payload_len) < 0) return -1;
    }
    return 0;
}

int recv_frame(int fd, void* hdr_out, void* payload_buf, uint16_t buf_cap, uint16_t* out_len) {
    uint16_t be_total = 0;
    if (readn_all(fd, &be_total, sizeof(be_total)) < 0) return -1;
    uint16_t total = ntohs(be_total);
    if (total < sizeof(msg_header_t)) return -1;
    if (readn_all(fd, hdr_out, sizeof(msg_header_t)) < 0) return -1;
    uint16_t payload_len = (uint16_t)(total - sizeof(msg_header_t));
    if (payload_len > buf_cap) return -1;
    if (payload_len > 0) {
        if (readn_all(fd, payload_buf, payload_len) < 0) return -1;
    }
    if (out_len) *out_len = payload_len;
    return 0;
}
