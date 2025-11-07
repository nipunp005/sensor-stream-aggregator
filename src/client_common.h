/************************************************************
 * File: client_common.h
 * Project: sensor-stream-aggregator
 * Description:
 *   Common helper functions for socket handling, timing,
 *   and control protocol operations used by client programs.
 ************************************************************/

#ifndef CLIENT_COMMON_H
#define CLIENT_COMMON_H

#include <stdint.h>
#include <stddef.h>

// Establish a non-blocking TCP connection to host:port.
int connect_nonblocking(const char *host, int port);
// Returns current time in milliseconds since Unix epoch.
uint64_t now_ms(void);
// Reads data from a TCP socket and extracts the latest complete line token.
int read_extract_latest(int fd, char *buf, char *token_out, size_t token_len);
// Safely closes a file descriptor if valid, then sets it to -1.
void close_fd(int *fdp);
// Connect to TCP sockets
int connect_all_sockets(const char *host, const int *ports, int count, int *sock);
// Print json on stdout
void print_json(unsigned long long ts, char last_val[][64], int count);
// Find the maxfd value
int maxfd_func(const int *sock, int count);


#endif