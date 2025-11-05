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

/* Establish TCP connection to given host and port. */
static int connect_to(const char *host, int port) {
    int s = socket(AF_INET,SOCK_STREAM,0);
    if(s < 0) { 
        perror("socket");
        exit(1);
    }

    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    inet_pton(AF_INET, host, &sa.sin_addr);

    if(connect(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("connect");
        close(s);
        return -1;
    }

    return s;
}

int main(void) {

    const char *host = "127.0.0.1";
    int ports[3] = {4001, 4002, 4003};
    int sock[3];

    /* Connect to all three ports. */
    for(int i=0; i<3; i++) {
        sock[i] = connect_to(host, ports[i]);
        if(sock[i] < 0) return 1;
        // Debug messages.
        fprintf(stderr, "Connected to port %d\n", ports[i]);
    }

    fd_set rfds;
    char buf[1024];
    int maxfd = sock[0];
    for(int i=0; i<3 ; i++) {
        if(sock[i] > maxfd) {
            maxfd = sock[i];
        }
    }

     /* Main loop: wait for data on any port and print it. */
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

        /* Read from whichever socket has data ready. */
        for(int i=0; i<3; i++) {
            if(FD_ISSET(sock[i], &rfds)) {
                ssize_t r = read(sock[i], buf, sizeof(buf)-1);
                if(r<=0) {
                    fprintf(stderr, "Port %d closed or error\n", ports[i]);
                    close(sock[i]);
                    sock[i] = connect_to(host, ports[i]);
                    continue;
                }
                buf[r] = '\0';
                printf("[port:%d] %s", ports[i], buf);
                fflush(stdout);
            }
        }

    }

}