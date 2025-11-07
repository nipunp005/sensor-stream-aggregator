/************************************************************
 * File: client2.c
 * Project: sensor-stream-aggregator
 * Description:
 *   Closed-loop control client for sensor stream server.
 *
 *   - Reads data from TCP ports 4001, 4002, 4003.
 *   - Prints JSON every 20 ms.
 *   - Monitors out3; adjusts out1 frequency/amplitude via UDP control:
 *        out3 >= 3.0 → freq=1Hz, amp=8000
 *        out3 <  3.0 → freq=2Hz, amp=4000
 *
 * Author: Nipun Pal
 * Date: 2025-11-06
 ************************************************************/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>
#include <stdint.h>
#include "client_common.h"

#define CONTROL_PORT 4000
#define OUT_PORTS_COUNT 3
#define WINDOW_MS 20

// Property IDs
#define PROP_ENABLE     14
#define PROP_AMPLITUDE  170
#define PROP_FREQUENCY  255

// Command IDs
#define READ_OP  1
#define WRITE_OP 2

// Helper : Send UDP write
static void send_write(int udp_fd, struct sockaddr_in *addr, uint16_t obj, uint16_t property, uint16_t val)
{
    uint16_t msg[4];
    msg[0] = htons(WRITE_OP);
    msg[1] = htons(obj);
    msg[2] = htons(property);
    msg[3] = htons(val);

    // Attempt to send UDP packet
    ssize_t sent = sendto(udp_fd, msg, sizeof(msg), 0, (struct sockaddr *)addr, sizeof(*addr));

    // Retry once if initial transmission fails
    if (sent != sizeof(msg)) {
        perror("UDP sendto failed");
        sleep(2);
        sendto(udp_fd, msg, sizeof(msg), 0, (struct sockaddr *)addr, sizeof(*addr));
    }
}

int main(void)
{
    const char *host = "127.0.0.1";
    int ports[OUT_PORTS_COUNT] = {4001, 4002, 4003};
    int sock[OUT_PORTS_COUNT];
    char buf[OUT_PORTS_COUNT][64];
    char token[OUT_PORTS_COUNT][64];
    char last_val[OUT_PORTS_COUNT][64] = {"--", "--", "--"};

    // Connect all TCP sockets
    if(connect_all_sockets(host, ports, OUT_PORTS_COUNT, sock) < 0)
        return 1;

    // Setup UDP socket for control
    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) {
        perror("udp socket");
        return 1;
    }

    struct sockaddr_in ctrl_addr = {0};
    ctrl_addr.sin_family = AF_INET;
    ctrl_addr.sin_port = htons(CONTROL_PORT);
    inet_pton(AF_INET, host, &ctrl_addr.sin_addr);

    // Initialize and set defaults
    unsigned long long last_tick = now_ms();
    int last_state = -1;
    double out3_val = 0.0;

    // Prepare select() read set for non-blocking I/O monitoring.
    fd_set rfds;
    int maxfd = sock[0];
    for (int i = 1; i < OUT_PORTS_COUNT; ++i)
        if (sock[i] > maxfd)
            maxfd = sock[i];

    // Main loop: poll sockets, update readings, evaluate control logic
    while (1) {
        FD_ZERO(&rfds);
        for (int i = 0; i < OUT_PORTS_COUNT; ++i)
            if (sock[i] >= 0)
                FD_SET(sock[i], &rfds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 5000;  // check every 5 ms

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
                int r = read_extract_latest(sock[i], buf[i], token[i], sizeof(token[i]));
                if (r == 1) {
                    snprintf(last_val[i], sizeof(last_val[i]), "%s", token[i]);
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

        // Every WINDOW_MS (20 ms), evaluate control state and print JSON.
        unsigned long long now = now_ms();
        if (now - last_tick >= WINDOW_MS) {
            // Evaluate control condition
            if (strcmp(last_val[2], "--") != 0)
                out3_val = atof(last_val[2]);

            int state = (out3_val >= 3.0) ? 1 : 0;

            if (state != last_state) {
                if (state == 1) {
                    /* Ideally we could do a range validation here for both frequency and amplitude
                       since here it is very much tied to implementation logic we are directly passing it
                       If this was coming from some external module then definately range checks could be performed for sure
                    */
                    send_write(udp_fd, &ctrl_addr, 1, PROP_FREQUENCY, 500);
                    send_write(udp_fd, &ctrl_addr, 1, PROP_AMPLITUDE, 8000);
                } else {
                    send_write(udp_fd, &ctrl_addr, 1, PROP_FREQUENCY, 1000);
                    send_write(udp_fd, &ctrl_addr, 1, PROP_AMPLITUDE, 4000);
                }
                last_state = state;
            }

            // Print one JSON object
            print_json(now, last_val, OUT_PORTS_COUNT);

            // Reset tick
            last_tick = now;

            // Reset missing ports
            for (int i = 0; i < OUT_PORTS_COUNT; i++)
                strcpy(last_val[i], "--");
        }
    }

    // Cleanup
    for (int i = 0; i < OUT_PORTS_COUNT; i++)
        close_fd(&sock[i]);

    close(udp_fd);
    return 0;
}
