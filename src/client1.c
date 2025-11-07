/************************************************************
 * File: client1.c
 * Project: sensor-stream-aggregator
 * Description:
 *   Client that connects to TCP ports (4001, 4002, 4003),
 *   collects values every 100 ms, and prints structured JSON
 *   lines to stdout.
 *   Each line contains timestamp (ms since epoch) and the most
 *   recent values for out1/out2/out3, or "--" if missing.
 *
 * Author: Nipun Pal
 * Date: 2025-11-06
 ************************************************************/

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include "client_common.h"

#define WINDOW_MS 100
#define OUT_PORTS_COUNT 3

int main(void) {

    const char *host = "127.0.0.1";
    int ports[OUT_PORTS_COUNT] = {4001, 4002, 4003};
    int sock[OUT_PORTS_COUNT];

    // latest known values (as strings)
    char last_val[OUT_PORTS_COUNT][64] = {"--","--","--"};
        
    char buf[OUT_PORTS_COUNT][512];
    char token[64];

    // connect all sockets using helper
    for(int i=0; i<OUT_PORTS_COUNT; i++) {
        sock[i] = connect_nonblocking(host, ports[i]);
        if(sock[i] < 0) {
            fprintf(stderr, "Connection to port %d failed\n", ports[i]);
            return 1;
        }
        fprintf(stderr, "Connected to port : %d\n", ports[i]);
    }

    int maxfd = sock[0];
    for (int i = 1; i < OUT_PORTS_COUNT; ++i)
        if (sock[i] > maxfd) maxfd = sock[i];

    fd_set rfds;

    // start the timer
    unsigned long long last_tick = now_ms();

    while(1) {

        // set up read fd set
        FD_ZERO(&rfds);
        for (int i = 0; i < OUT_PORTS_COUNT; ++i)
            if (sock[i] >= 0)
                FD_SET(sock[i], &rfds);

        // timeout tuned for 100ms loop precision
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 1000 * 5; // check every 5ms for new data

        int ready = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (ready < 0) {
            if (errno == EINTR)
                continue;
            perror("select");
            break;
        }

        // Read data from all ready sockets
        for (int i = 0; i < OUT_PORTS_COUNT; ++i) {
            if (sock[i] >= 0 && FD_ISSET(sock[i], &rfds)) {
                int r = read_extract_latest(sock[i], buf[i], token, sizeof(token));
                if (r == 1) {
                    snprintf(last_val[i], sizeof(last_val[i]), "%s", token);
                    last_val[i][sizeof(last_val[i]) - 1] = '\0';
                } else if (r < 0) {
                    fprintf(stderr, "Port %d closed or error, reconnecting...\n", ports[i]);
                    close_fd(&sock[i]);
                    sock[i] = connect_nonblocking(host, ports[i]);
                    if (sock[i] > maxfd)
                        maxfd = sock[i];
                }
            }
        }

        // Check if 100ms window elapsed
        unsigned long long now = now_ms();
        if(now - last_tick >= WINDOW_MS) {
            printf("{\"timestamp\": %llu, \"out1\": \"%s\", \"out2\": \"%s\", \"out3\": \"%s\"}\n",
                    now, last_val[0], last_val[1], last_val[2]);
            fflush(stdout);

            // reset tick
            last_tick = now;

            // reset missing ports to "--" for next window
            for(int i=0; i<3; i++) {
                strcpy(last_val[i], "--");
            }
        }
    }

    // cleanup
    for(int i=0; i<OUT_PORTS_COUNT; i++) {
        close_fd(&sock[i]);
    }

    return 0;
}