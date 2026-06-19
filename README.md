# Smart Device Dev

A web-based IDE for Arduino and embedded device development. Provides device discovery, a code editor, compilation & upload, serial monitoring, and I2C scanning — all through a browser interface.

## Features

- **Device Discovery** — Auto-detects Arduino, ESP32, Raspberry Pi Pico boards by USB VID/PID
- **Code Editor** — Browser-based C/C++ editor with Arduino syntax highlighting and dark theme
- **Compile & Upload** — Compile and upload sketches via `arduino-cli`
- **Serial Monitor** — Real-time serial terminal via WebSocket with configurable baud rate
- **I2C/GPIO Scanner** — Scan I2C buses and identify connected chips/sensors by address

## Tech Stack

| Layer     | Technology                          |
|-----------|-------------------------------------|
| Backend   | Python 3, FastAPI, Uvicorn          |
| Frontend  | Vue 3, Vite, CodeMirror 6           |
| Comms     | REST API, WebSockets                |
| Tools     | arduino-cli, pyserial, i2c-tools    |

## Prerequisites

- **Python 3** with `pip`
- **Node.js** (any recent version)
- **arduino-cli** (optional, for compile/upload)
- **i2c-tools** (optional, Linux only, for I2C scanning)

## Quick Start

```bash
./start.sh
```

This launches both the backend (port 8720) and frontend dev server (port 5173). Press `Ctrl+C` to stop.

## Manual Setup

### Backend

```bash
cd backend
pip install -r requirements.txt
python3 main.py
```

Runs on `http://localhost:8720`.

### Frontend

```bash
cd frontend
npm install
npm run dev        # dev server on :5173
npm run build      # production build to dist/
npm run preview    # preview production build
```

## Architecture

```
start.sh                          # One-command launcher
backend/
  main.py                         # FastAPI app (REST + WebSocket endpoints)
  board_detector.py               # USB VID/PID board detection
  serial_manager.py               # Serial connection management
  i2c_scanner.py                  # I2C bus scanning
  requirements.txt
frontend/
  src/
    App.vue                       # Root layout, tabs, keyboard shortcuts
    api/index.js                  # HTTP + WebSocket client helpers
    components/
      BoardDetector.vue           # Device discovery sidebar
      CodeEditor.vue              # CodeMirror editor + compile/upload
      SerialMonitor.vue           # WebSocket serial terminal
      GpioScanner.vue             # I2C bus scanner UI
  vite.config.js                  # Proxies /api and /ws to backend
examples/                         # Bundled Arduino example sketches
```

The Vite dev server proxies `/api` and `/ws` requests to the Python backend at `localhost:8720`. CORS allows all origins for development convenience.

## License

MIT
