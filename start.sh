#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BACKEND_DIR="$SCRIPT_DIR/backend"
FRONTEND_DIR="$SCRIPT_DIR/frontend"

echo "============================================"
echo "  Smart Device Dev - Startup"
echo "============================================"

# Check Python deps
echo ""
echo "[1/3] Checking Python dependencies..."
python3 -c "import fastapi, uvicorn, serial, websockets" 2>/dev/null && echo "  OK: Python deps satisfied" || {
    echo "  Missing Python dependencies. Installing..."
    pip3 install --break-system-packages fastapi uvicorn pyserial websockets python-dotenv
}

# Check Node
echo ""
echo "[2/3] Checking Node.js..."
node --version >/dev/null 2>&1 && echo "  OK: Node $(node --version)" || {
    echo "  ERROR: Node.js required. Install: https://nodejs.org"
    exit 1
}

# Install frontend deps
echo ""
echo "[3/3] Installing frontend dependencies..."
cd "$FRONTEND_DIR"
npm install --silent 2>/dev/null || npm install

echo ""
echo "============================================"
echo "  Starting services..."
echo "============================================"
echo "  Backend:  http://localhost:8720"
echo "  Frontend: http://localhost:5173"
echo ""

# Start backend
cd "$BACKEND_DIR"
python3 main.py &
BACKEND_PID=$!

# Start frontend dev server
cd "$FRONTEND_DIR"
npx vite --host &
FRONTEND_PID=$!

trap "kill $BACKEND_PID $FRONTEND_PID 2>/dev/null; exit" INT TERM

echo ""
echo "Press Ctrl+C to stop both services"
wait
