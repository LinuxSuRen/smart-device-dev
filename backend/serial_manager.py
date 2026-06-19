import serial
import asyncio
import threading
from typing import Optional, Callable


class SerialConnection:
    def __init__(self, port: str, baudrate: int = 9600):
        self.port = port
        self.baudrate = baudrate
        self._serial: Optional[serial.Serial] = None
        self._running = False
        self._thread: Optional[threading.Thread] = None
        self._on_data: Optional[Callable[[str], None]] = None
        self._loop = None

    def open(self) -> bool:
        try:
            self._serial = serial.Serial(self.port, self.baudrate, timeout=0.1)
            return True
        except Exception as e:
            print(f"Failed to open {self.port}: {e}")
            return False

    def close(self):
        self._running = False
        if self._serial and self._serial.is_open:
            self._serial.close()
        self._serial = None

    def start_reading(self, callback: Callable[[str], None]):
        self._on_data = callback
        self._running = True
        self._thread = threading.Thread(target=self._read_loop, daemon=True)
        self._thread.start()

    def _read_loop(self):
        while self._running and self._serial and self._serial.is_open:
            try:
                if self._serial.in_waiting > 0:
                    data = self._serial.read(self._serial.in_waiting)
                    text = data.decode("utf-8", errors="replace")
                    print(f"[serial] read {len(text)} bytes from {self.port}")
                    if self._on_data:
                        self._on_data(text)
            except Exception as e:
                print(f"[serial] read error: {e}")
            import time
            time.sleep(0.05)

    def write(self, data: str) -> bool:
        try:
            if self._serial and self._serial.is_open:
                self._serial.write(data.encode("utf-8", errors="replace"))
                return True
        except Exception:
            pass
        return False

    def is_open(self) -> bool:
        return self._serial is not None and self._serial.is_open


class SerialManager:
    def __init__(self):
        self._connections: dict[str, SerialConnection] = {}

    def connect(self, port: str, baudrate: int = 9600) -> bool:
        if port in self._connections:
            return True
        conn = SerialConnection(port, baudrate)
        if conn.open():
            self._connections[port] = conn
            return True
        return False

    def disconnect(self, port: str):
        if port in self._connections:
            self._connections[port].close()
            del self._connections[port]

    def disconnect_all(self):
        for port in list(self._connections.keys()):
            self.disconnect(port)

    def get_connection(self, port: str) -> Optional[SerialConnection]:
        return self._connections.get(port)

    def write(self, port: str, data: str) -> bool:
        conn = self._connections.get(port)
        if conn:
            return conn.write(data)
        return False
