import sys
import os
import subprocess
import json
import asyncio
import platform
from typing import Optional

# --- Dependency checks at module level ---
_missing_deps = []
_serial_available = True
_fastapi_available = True
_websockets_available = True

try:
    from fastapi import FastAPI, WebSocket, WebSocketDisconnect, Body
    from fastapi.middleware.cors import CORSMiddleware
    from pydantic import BaseModel
except ImportError:
    _fastapi_available = False
    _missing_deps.append("fastapi uvicorn pydantic")

try:
    import serial
except ImportError:
    _serial_available = False
    _missing_deps.append("pyserial")

try:
    import websockets
except ImportError:
    _websockets_available = False
    _missing_deps.append("websockets")


SYSTEM_DEPS_INSTALL_HELP = {
    "fastapi": "pip install fastapi uvicorn pydantic",
    "pyserial": "pip install pyserial",
    "websockets": "pip install websockets",
    "i2c-tools": "sudo apt install i2c-tools",
    "arduino-cli": "curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh",
}


def _install_pip_deps(deps: list[str]):
    for dep in deps:
        print(f"[setup] Installing {dep}...")
        subprocess.run([sys.executable, "-m", "pip", "install", *dep.split()], check=False)


def check_startup_deps():
    issues = []
    warnings = []

    # Python packages
    for dep_name, module_name in [("fastapi", "fastapi"), ("pyserial", "serial"), ("websockets", "websockets"), ("uvicorn", "uvicorn")]:
        try:
            __import__(module_name)
        except ImportError:
            issues.append(dep_name)

    # System tools
    for tool, name in [("arduino-cli", "arduino-cli"), ("i2cdetect", "i2c-tools")]:
        if subprocess.run(["which", tool], capture_output=True).returncode != 0:
            warnings.append(name)

    # Linux-specific checks
    system = platform.system()
    if system == "Linux":
        if not os.path.exists("/dev/i2c-1") and not os.path.exists("/dev/i2c-0"):
            warnings.append("i2c-dev (kernel module not loaded: sudo modprobe i2c-dev)")
    else:
        warnings.append(f"I2C/GPIO scanning only supported on Linux (current: {system})")

    return issues, warnings


# If running directly, ensure deps are available
if __name__ == "__main__":
    issues, warnings = check_startup_deps()
    if issues:
        print(f"\n{'='*60}")
        print("[setup] Missing Python dependencies detected:")
        for dep in issues:
            cmd = SYSTEM_DEPS_INSTALL_HELP.get(dep, f"pip install {dep}")
            print(f"  - {dep}: {cmd}")
        print(f"{'='*60}")
        if os.isatty(sys.stdin.fileno()):
            try:
                ans = input("\nAuto-install missing dependencies? [Y/n]: ").strip().lower()
                if ans in ("", "y", "yes"):
                    _install_pip_deps(issues)
                    print("[setup] Dependencies installed. Please restart the application.")
                    sys.exit(1)
                else:
                    print("[setup] Please install manually and restart.")
                    sys.exit(1)
            except EOFError:
                pass
        else:
            print("[setup] Run interactively to auto-install, or install manually:")
            for dep in issues:
                cmd = SYSTEM_DEPS_INSTALL_HELP.get(dep, f"pip install {dep}")
                print(f"  {cmd}")
            sys.exit(1)

    if warnings:
        print(f"\n[setup] Warnings:")
        for w in warnings:
            cmd = SYSTEM_DEPS_INSTALL_HELP.get(w, f"Install: {w}")
            print(f"  - {w}  ({cmd})")
        print()

# If imported as module but deps missing, raise clean error
if not _fastapi_available and __name__ != "__main__":
    raise ImportError(
        "Missing: fastapi, uvicorn, pydantic. Run: pip install fastapi uvicorn pydantic"
    )


from board_detector import detect_boards, get_arduino_cli_info
from serial_manager import SerialManager
from i2c_scanner import scan_i2c_bus, list_i2c_buses, detect_chip_info

app = FastAPI(title="Smart Device Dev", version="1.0.0")
serial_manager = SerialManager()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


class SerialConnectRequest(BaseModel):
    port: str
    baudrate: int = 9600


class SerialWriteRequest(BaseModel):
    port: str
    data: str


class I2CScanRequest(BaseModel):
    address: Optional[int] = None
    bus: int = 1


@app.get("/api/health")
async def health():
    issues, warnings = check_startup_deps()
    return {
        "status": "ok",
        "python_issues": issues,
        "system_warnings": warnings,
        "system": platform.system(),
    }


@app.get("/api/boards")
async def get_boards():
    boards = detect_boards() if _serial_available else []
    cli_info = get_arduino_cli_info()
    return {
        "boards": boards,
        "arduino_cli": cli_info,
        "serial_available": _serial_available,
    }


@app.get("/api/deps")
async def get_deps():
    issues, warnings = check_startup_deps()
    return {
        "python_deps_ok": len(issues) == 0,
        "missing_python": issues,
        "missing_system": warnings,
        "install_commands": {
            k: SYSTEM_DEPS_INSTALL_HELP[k]
            for k in issues + warnings
            if k in SYSTEM_DEPS_INSTALL_HELP
        },
    }


@app.post("/api/serial/connect")
async def serial_connect(req: SerialConnectRequest):
    if not _serial_available:
        return {"success": False, "error": "pyserial not installed", "install_cmd": SYSTEM_DEPS_INSTALL_HELP["pyserial"]}
    success = serial_manager.connect(req.port, req.baudrate)
    return {"success": success, "port": req.port}


@app.post("/api/serial/disconnect")
async def serial_disconnect(req: SerialConnectRequest):
    serial_manager.disconnect(req.port)
    return {"success": True, "port": req.port}


@app.post("/api/serial/write")
async def serial_write(req: SerialWriteRequest):
    if not _serial_available:
        return {"success": False, "error": "pyserial not installed"}
    success = serial_manager.write(req.port, req.data)
    return {"success": success}


@app.get("/api/serial/status")
async def serial_status():
    statuses = {}
    for port, conn in serial_manager._connections.items():
        statuses[port] = conn.is_open()
    return {"connections": statuses}


@app.websocket("/ws/serial/{port}")
async def websocket_serial(websocket: WebSocket, port: str):
    await websocket.accept()
    full_port = f"/dev/{port}" if not port.startswith("/dev/") else port
    conn = serial_manager.get_connection(full_port)
    if not conn and port != full_port:
        conn = serial_manager.get_connection(port)
    if not conn:
        await websocket.send_text(json.dumps({"type": "error", "data": "Not connected to port"}))
        await websocket.close()
        return

    queue = asyncio.Queue()
    loop = asyncio.get_event_loop()

    def on_data(text: str):
        loop.call_soon_threadsafe(queue.put_nowait, text)

    conn.start_reading(on_data)

    try:
        read_task = asyncio.create_task(_read_serial(queue, websocket))
        write_task = asyncio.create_task(_write_serial(websocket, conn))
        await asyncio.gather(read_task, write_task)
    except WebSocketDisconnect:
        pass
    finally:
        conn._running = False


async def _read_serial(queue: asyncio.Queue, websocket: WebSocket):
    try:
        while True:
            data = await asyncio.wait_for(queue.get(), timeout=30)
            await websocket.send_text(json.dumps({"type": "data", "data": data}))
    except (asyncio.TimeoutError, WebSocketDisconnect):
        pass


async def _write_serial(websocket: WebSocket, conn):
    try:
        while True:
            msg = await websocket.receive_text()
            conn.write(msg)
    except WebSocketDisconnect:
        pass


@app.post("/api/i2c/scan")
async def i2c_scan(req: I2CScanRequest):
    if req.address is not None:
        info = detect_chip_info(req.address)
        return {"address": f"0x{req.address:02X}", "chip": info}
    devices = scan_i2c_bus(req.bus)
    return {"devices": devices, "bus": req.bus}


@app.get("/api/i2c/buses")
async def get_i2c_buses():
    buses = list_i2c_buses()
    return {"buses": buses}


EXAMPLES_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "examples")


@app.get("/api/examples")
async def list_examples():
    if not os.path.isdir(EXAMPLES_DIR):
        return {"examples": []}
    items = []
    for entry in sorted(os.listdir(EXAMPLES_DIR)):
        full = os.path.join(EXAMPLES_DIR, entry)
        if os.path.isdir(full):
            ino_file = os.path.join(full, f"{entry}.ino")
            if os.path.isfile(ino_file):
                items.append({"name": entry, "file": f"{entry}.ino"})
        elif entry.endswith(".ino"):
            name = entry[:-4]
            items.append({"name": name, "file": entry})
    return {"examples": items}


@app.get("/api/examples/{name}")
async def get_example(name: str):
    ino_file = os.path.join(EXAMPLES_DIR, f"{name}.ino")
    if os.path.isfile(ino_file):
        with open(ino_file, "r") as f:
            content = f.read()
        return {"success": True, "name": name, "content": content}
    d = os.path.join(EXAMPLES_DIR, name)
    if os.path.isdir(d):
        ino_file = os.path.join(d, f"{name}.ino")
        if os.path.isfile(ino_file):
            with open(ino_file, "r") as f:
                content = f.read()
            return {"success": True, "name": name, "content": content}
    return {"success": False, "error": f"Example '{name}' not found"}


@app.post("/api/compile")
async def compile_code(code: str = Body(""), board: str = Body("arduino:avr:uno")):
    import tempfile

    try:
        body = code
    except Exception:
        return {"success": False, "error": "No code provided"}

    cli_info = get_arduino_cli_info()
    if not cli_info["available"]:
        return {"success": False, "error": "arduino-cli not found", "install_cmd": SYSTEM_DEPS_INSTALL_HELP["arduino-cli"]}

    with tempfile.TemporaryDirectory() as tmpdir:
        sketch_dir = os.path.join(tmpdir, "sketch")
        os.makedirs(sketch_dir)
        sketch_path = os.path.join(sketch_dir, "sketch.ino")
        with open(sketch_path, "w") as f:
            f.write(body)

        try:
            result = subprocess.run(
                ["arduino-cli", "compile", "--fqbn", board, sketch_dir],
                capture_output=True, text=True, timeout=60,
            )
            return {
                "success": result.returncode == 0,
                "output": result.stdout,
                "error": result.stderr if result.returncode != 0 else None,
            }
        except subprocess.TimeoutExpired:
            return {"success": False, "error": "Compilation timed out"}
        except Exception as e:
            return {"success": False, "error": str(e)}


@app.post("/api/upload")
async def upload_code(code: str = Body(""), board: str = Body("arduino:avr:uno"), port: str = Body("")):
    import tempfile

    try:
        body = code
    except Exception:
        return {"success": False, "error": "No code provided"}

    cli_info = get_arduino_cli_info()
    if not cli_info["available"]:
        return {"success": False, "error": "arduino-cli not found", "install_cmd": SYSTEM_DEPS_INSTALL_HELP["arduino-cli"]}

    with tempfile.TemporaryDirectory() as tmpdir:
        sketch_dir = os.path.join(tmpdir, "sketch")
        os.makedirs(sketch_dir)
        sketch_path = os.path.join(sketch_dir, "sketch.ino")
        with open(sketch_path, "w") as f:
            f.write(body)

        cmd = ["arduino-cli", "compile", "--upload", "--fqbn", board]
        if port:
            cmd.extend(["-p", port])
        cmd.append(sketch_dir)

        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
            return {
                "success": result.returncode == 0,
                "output": result.stdout,
                "error": result.stderr if result.returncode != 0 else None,
            }
        except subprocess.TimeoutExpired:
            return {"success": False, "error": "Upload timed out"}
        except Exception as e:
            return {"success": False, "error": str(e)}


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8720)
