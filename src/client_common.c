/************************************************************
 * File: client_common.c
 * Project: sensor-stream-aggregator
 * Description:
 *   Implements helper functions for non-blocking socket
 *   creation, timestamp retrieval, safe closing, and line
 *   extraction from TCP streams.
 ************************************************************/


#define _POSIX_C_SOURCE 200809L
#include "client_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <time.h>

#define LOCAL_BUF_MAX 4096

/* Establish a non-blocking TCP connection to host:port. */
int connect_nonblocking(const char *host, int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return -1;
    }

    /* set non-blocking flag */
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0 || fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl");
        close(s);
        return -1;
    }

    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &sa.sin_addr) <= 0) {
        perror("inet_pton");
        close(s);
        return -1;
    }

    /* initiate non-blocking connect */
    connect(s, (struct sockaddr *)&sa, sizeof(sa));
    return s; /* connection in progress or complete */
}

/* Return current time in milliseconds since Unix epoch. */
uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t time = ((uint64_t)ts.tv_sec * 1000ULL) + (ts.tv_nsec / 1000000ULL);
    return time;
}

/* Read from TCP socket and extract latest complete line (trimmed). */
int read_extract_latest(int fd, char *buf, char *token_out, size_t token_len) {

    ssize_t r = recv(fd, buf, token_len-1, 0);
    if(r < 0) {
        // No data yet – non-blocking socket would return EAGAIN/EWOULDBLOCK
        if(errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        perror("recv");
        return -1;
    } else if (r == 0) {
        return -1; // Connection closed gracefully
    }

    buf[r] = '\0';

     // Trim trailing newline and carriage return characters
    for(ssize_t i = r-1; i>=0; i--) {
        if(buf[i] == '\n' || buf[i] == '\r') 
            buf[i] = '\0';
        else
            break;
    }

    // copy clean token into output buffer
    strncpy(token_out, buf, token_len-1);
    token_out[token_len-1] = '\0';
    return 1;
}

/* Safely close a file descriptor and reset it to -1. */
void close_fd(int *fdp) {
    if(fdp && (*fdp >= 0)) {
        close(*fdp);
        *fdp = -1;
    }
}

