CC=gcc
CFLAGS=-Wall -Wextra -O2 -std=c11 -I./shared/include
LDFLAGS=

SHARED_OBJS=shared/src/protocol.o

SERVER_OBJS=server/src/main.o
CLIENT_OBJS=client/src/main.o

all: server client

server: $(SHARED_OBJS) $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o server_bin $^ $(LDFLAGS)

client: $(SHARED_OBJS) $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o client_bin $^ $(LDFLAGS)

clean:
	rm -f $(SHARED_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS) server_bin client_bin

.PHONY: all server client clean
