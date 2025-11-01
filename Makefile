\
# Makefile for TCP Echo Client/Server (Linux)
# Student: 况旻諭 (614430005)

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -pedantic -std=c11

all: tcp_server tcp_client

tcp_server: 614430005_TCPServer.c
	$(CC) $(CFLAGS) -o $@ $<

tcp_client: 614430005_TCPClient.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f tcp_server tcp_client

.PHONY: all clean
