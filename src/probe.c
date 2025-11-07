/************************************************************
 * File: probe.c
 * Project: sensor-stream-aggregator
 * Description:
 *   Simple TCP listener that connects to three server ports
 *   (4001, 4002, 4003) and dumps all incoming data to the
 *   terminal for inspection.
 * Author: Nipun Pal
 * Date: 2025-11-04
 ************************************************************/

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>
#include "client_common.h"   // common helpers

int main(void) {

    const char *host = "127.0.0.1";
    int ports[3] = {4001, 4002, 4003};
    int sock[3];

    // Connect to all three ports using reusable helper
    for (int i = 0; i < 3; i++) {
        sock[i] = connect_nonblocking(host, ports[i]);
        if (sock[i] < 0) return 1;
        fprintf(stderr, "Connected to port %d\n", ports[i]);
    }

    fd_set rfds;
    char buf[1024];
    char token[256];

    int maxfd = sock[0];
    for (int i = 1; i < 3; i++) {
        if (sock[i] > maxfd)
            maxfd = sock[i];
    }

     // Main loop: wait for data on any port and print it.
    while(1) {
        
        FD_ZERO(&rfds);
        for(int i=0; i<3; i++) {
            FD_SET(sock[i], &rfds);
        }

        if(select(maxfd+1, &rfds, NULL, NULL, NULL) < 0) {
            if(errno == EINTR) continue;
            perror("Select");
            break;
        }

        // Read from whichever socket has data ready
        for (int i = 0; i < 3; i++) {
            if (sock[i] >= 0 && FD_ISSET(sock[i], &rfds)) {
                int r = read_extract_latest(sock[i], buf, token, sizeof(token));
                if (r == 1) {
                    printf("[port:%d] %s\n", ports[i], token);
                    fflush(stdout);
                } else if (r < 0) {
                    fprintf(stderr, "Port %d closed or error\n", ports[i]);
                    close_fd(&sock[i]);
                    sock[i] = connect_nonblocking(host, ports[i]);
                    if (sock[i] >= 0 && sock[i] > maxfd)
                        maxfd = sock[i];
                }
            }
        }
    }
}