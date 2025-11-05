#!/usr/bin/env python3
import re
import matplotlib.pyplot as plt

# Input log file from probe
LOG_FILE = "../src/sensor_dump.txt"

# Pattern: [port4001] -0.2  or [port:4001] -0.2
line_re = re.compile(r"\[port[:]?(\d+)\]\s*([-+]?\d*\.?\d+)")

# Data dictionary keyed by port
data = {
    4001: [],
    4002: [],
    4003: []
}

with open(LOG_FILE) as f:
    for line in f:
        m = line_re.search(line)
        if m:
            port = int(m.group(1))
            val = float(m.group(2))
            if port in data:
                data[port].append(val)

# Sanity check: number of samples
for p, vals in data.items():
    print(f"Port {p}: {len(vals)} samples")

# Build time axis (assuming uniform sample rate, just use index)
fig, axes = plt.subplots(3, 1, figsize=(10, 7))
fig.suptitle("Sensor Output from Probe for 60s (Ports 4001–4003)")

for idx, (port, vals) in enumerate(sorted(data.items())):
    ax = axes[idx]
    ax.plot(range(len(vals)), vals, label=f"Port {port}")
    ax.set_ylabel("Value")
    ax.legend(loc="upper right")
    ax.grid(True)

axes[-1].set_xlabel("Sample Index (time)")
plt.tight_layout()
plt.show()
