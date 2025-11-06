#!/usr/bin/env python3
"""
plot.py

Each port’s data is plotted on its own subplot with independent X scaling.

Usage:
    python3.py <filename>
"""

import sys
import re
import json
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

def parse_probe_txt(filepath):
    """Parse output from probe.c"""
    ports = {4001: [], 4002: [], 4003: []}
    with open(filepath, "r") as f:
        for line in f:
            m = re.match(r"\[port:(\d+)\]\s+(-?\d+\.?\d*)", line.strip())
            if m:
                port = int(m.group(1))
                val = float(m.group(2))
                ports[port].append(val)
    return ports, None  # no timestamps, just sequential index


def parse_client_json(filepath):
    """Parse JSON output from client1.c"""
    out1, out2, out3, ts = [], [], [], []
    with open(filepath, "r") as f:
        for line in f:
            try:
                obj = json.loads(line)
                ts.append(obj.get("timestamp"))
                def safe(v):
                    try:
                        if v in (None, "--", ""):
                            return np.nan
                        return float(str(v).strip())
                    except Exception:
                        return np.nan
                
                out1.append(safe(obj.get("out1")))
                out2.append(safe(obj.get("out2")))
                out3.append(safe(obj.get("out3")))
            except (json.JSONDecodeError, TypeError, ValueError):
                continue
    df = pd.DataFrame({"timestamp": ts, "out1": out1, "out2": out2, "out3": out3})
    df.dropna(subset=["timestamp"], inplace=True)
    df.reset_index(drop=True, inplace=True)
    return df, "json"


def plot_ports(data, timestamps=None, json_mode=False):
    """Plot data from 3 ports with independent scaling"""
    fig, axes = plt.subplots(3, 1, figsize=(10, 8), sharex=False)
    fig.suptitle("Sensor Stream Comparison", fontsize=14)

    ports = [4001, 4002, 4003]
    labels = ["out1 (port 4001)", "out2 (port 4002)", "out3 (port 4003)"]

    if json_mode:
        t = (timestamps - timestamps[0]) / 1000.0  # ms → seconds
        axes[0].plot(t, data["out1"], label=labels[0])
        axes[1].plot(t, data["out2"], label=labels[1])
        axes[2].plot(t, data["out3"], label=labels[2])
    else:
        for i, port in enumerate(ports):
            y = data[port]
            x = np.arange(len(y)) / len(y)  # scaled X per dataset
            axes[i].plot(x, y, label=labels[i])

    for ax in axes:
        ax.legend(loc="upper right")
        ax.grid(True, linestyle="--", alpha=0.5)
        ax.set_ylabel("Value")

    axes[-1].set_xlabel("Time (s)" if json_mode else "Relative sample index")
    plt.tight_layout()
    plt.show()


def main():
    if len(sys.argv) != 2:
        print("Usage: python3 plot_sensor_data_scaled.py <file>")
        sys.exit(1)

    filepath = sys.argv[1]

    # Detect file type
    with open(filepath, "r") as f:
        head = f.readline()

    if head.strip().startswith("{"):
        df, _ = parse_client_json(filepath)
        plot_ports(df, df["timestamp"].to_numpy(), json_mode=True)
    else:
        ports, _ = parse_probe_txt(filepath)
        plot_ports(ports, json_mode=False)


if __name__ == "__main__":
    main()
