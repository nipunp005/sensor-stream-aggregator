#!/usr/bin/env python3
# send read probes for object in 1..3 and property in 1..256
import socket, struct, time, sys

if len(sys.argv) < 2:
    print("Usage: udp_probe_read.py <server_ip>")
    sys.exit(1)

server = (sys.argv[1], 4000)
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

for obj in range(1, 3):
    for prop in range(1, 256):
        msg = struct.pack('!HHH', 1, obj, prop)   # operation=1 read
        s.sendto(msg, server)
        print(f"Sent READ obj={obj} prop={prop}")
        time.sleep(0.15)
