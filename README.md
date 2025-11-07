# Sensor Stream Aggregator

**Author:** Nipun Pal
**Date:** November 2025
**Docs:** [`docs/module-design.pdf`](docs/module-design.pdf)

---

## Overview

This project implements a complete **client-side system** that communicates with the provided Embedded Linux homework server running inside a Docker container.
The system captures sensor waveforms over TCP, discovers the binary UDP control protocol, and implements a closed-loop controller based on live data feedback.

All implementation details, architecture diagrams, and analysis are available in the **`docs/module-design.pdf`** file.
This README provides a high-level summary.

---

## 1. Repository Layout

```
sensor-stream-aggregator/
├── Makefile                   # Build rules for probe, client1, client2
├── src/                       # C source files
│   ├── probe.c                # Basic TCP listener and sanity checker
│   ├── client1.c              # JSON logger (100 ms)
│   ├── client2.c              # Closed-loop controller (20 ms)
│   ├── client_common.c/.h     # Shared TCP utilities and timing helpers
├── tools/
│   ├── plot.py                # Visualizes JSON logs as waveforms
│   └── udp_probe.py           # UDP property discovery tool
├── results/                   # Captured logs and generated plots
│   ├── *.json, *.png          # Reference data and visual results
└── docs/
    └── module-design.pdf      # Full report with design, protocol, and analysis
```

---

## 2. Building the Project

The repository includes a simple GNU Make build system.

```bash
# Build all executables
make

# or individual targets
make probe
make client1
make client2

# Clean build directory
make clean
```

Binaries are generated under `build/`:
```
build/
 ├── probe
 ├── client1
 └── client2
```

No external dependencies are required — only a standard Linux environment with GCC and POSIX sockets.

---

## 3. Running the Server

Start the provided Docker container before running any client:

```bash
docker load < fsw-linux-homework.tar.gz
docker run -p 4000:4000/udp -p 4001:4001 -p 4002:4002 -p 4003:4003 fsw-linux-homework
```

This exposes:
- **TCP 4001 corresponds to out1**
- **TCP 4002 corresponds to out2**
- **TCP 4003 corresponds to out3**
- **UDP 4000 corresponds to control channel**

---

## 4. Components Summary

### `probe.c`
- Minimal TCP client used for initial signal inspection.
- Confirms ASCII-formatted numeric streams from server ports 4001–4003.

---

### `client1.c` — Data Aggregator
- Reads continuously from all three TCP streams.
- Every **100 ms**, prints one JSON object:
  ```json
  {"timestamp": 1709286246930, "out1": "-4.0", "out2": "--", "out3": "1.0"}
  ```
- Rules:
  - Uses the **most recent value** from each port within the 100 ms window.
  - If no data received corresponds to outputs `"--"`.
- Generates datasets for waveform visualization (`results/data-100ms-10min.json`).
- Result helped us understand that
  - `out1` :outputs sine wave
  - `out2` :outputs triangle wave
  - `out3` :outputs square wave
---

### `client2.c` — Closed-loop Controller
- Extends `client1` with a **20 ms loop** and **UDP-based control**.
- Logic:
  - If `out3 ≥ 3.0` corresponds to set `out1` frequency to 1 Hz and amplitude to 8000 mV.
  - If `out3 < 3.0` corresponds to set `out1` frequency to 2 Hz and amplitude to 4000 mV.
- Uses binary UDP protocol:
  ```
  [operation][object][property][value]
  ```
  - 16-bit unsigned fields, big-endian order.
  - Operation: 1=read, 2=write.
- Produces fine-grained logs (`results/data-client2-5mins.json`) confirming correct feedback behavior.

---

### `client_common.c / .h`
- Shared utilities for:
  - Non-blocking TCP reads
  - Timing and delay management
  - Reconnect and error recovery

---

### Tools

| Tool | Purpose |
|------|----------|
| `tools/udp_probe.py` | Iteratively tests UDP property IDs to map object–property–value relations. |
| `tools/plot.py` | Reads JSON logs and generates time-domain waveform plots. |

Example output plots can be found in `results/`:
- `data-viz-100ms-10min.png`
- `data-viz-client2-results-zoomedin.png`

---

## 5. Key Results

| Output | Shape | Approx. Frequency | Amplitude |
|---------|--------|------------------|------------|
| out1 | Sine | ≈ 1 Hz | (+/-)5 V |
| out2 | Triangle | ≈ 0.5 Hz | (+/-)5 V |
| out3 | Square | Period ≈ 8–10 s | 0–5 V |

Control verification (client2):
- Frequency and amplitude of **out1** toggle dynamically when **out3** crosses 3.0 V threshold.
- Plots confirm expected transitions and correct timing at 20 ms intervals.

---

## 6. Verification and Testing

- **UDP property mapping** confirmed via `udp_probe.py`.
- **Waveform consistency** validated against plotted data.
- **Timing accuracy** verified over multiple minutes (no observable drift).
- **Auto-reconnect** logic ensures stable TCP operation under transient drops.

---

## 7. Documentation

Full design rationale, architecture diagrams, protocol decoding, and analysis of signal behavior are included in:

**[`docs/module-design.pdf`](docs/module-design.pdf)**

This PDF expands on:
- Incremental development stages
- Detailed protocol mapping
- Observations and plots
- Correctness verification

---

## 8. Quick Start Recap

```bash
# 1. Start server
docker run -p 4000:4000/udp -p 4001:4001 -p 4002:4002 -p 4003:4003 fsw-linux-homework

# 2. Build clients
make

# 3. Run data logger (client1)
./build/client1 > results/data-client1.json

# 4. Run controller (client2)
./build/client2 > results/data-client2.json

# 5. Plot results
python3 tools/plot.py results/data-client1.json
python3 tools/plot.py results/data-client2.json

#6. See both the logs together
./build/client2 > output.json 2> debug.log

```

---

For in-depth system design, refer to **`docs/module-design.pdf`**.
