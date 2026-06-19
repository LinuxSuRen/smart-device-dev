import serial.tools.list_ports
import subprocess
import json
import os
import platform
from typing import Optional


def get_board_info(vid: int, pid: int) -> Optional[dict]:
    boards = {
        (0x2341, 0x0043): {"name": "Arduino Uno", "mcu": "ATmega328P"},
        (0x2341, 0x0001): {"name": "Arduino Uno (old)", "mcu": "ATmega328P"},
        (0x2341, 0x0010): {"name": "Arduino Mega 2560", "mcu": "ATmega2560"},
        (0x2341, 0x0042): {"name": "Arduino Mega ADK", "mcu": "ATmega2560"},
        (0x2341, 0x8036): {"name": "Arduino Leonardo", "mcu": "ATmega32U4"},
        (0x2341, 0x8037): {"name": "Arduino Micro", "mcu": "ATmega32U4"},
        (0x2341, 0x804E): {"name": "Arduino Nano", "mcu": "ATmega328P"},
        (0x2341, 0x8057): {"name": "Arduino Nano 33 IoT", "mcu": "SAMD21G18A"},
        (0x2341, 0x8058): {"name": "Arduino Nano 33 BLE", "mcu": "nRF52840"},
        (0x2341, 0x005E): {"name": "Arduino Due", "mcu": "ATSAM3X8E"},
        (0x2341, 0x804D): {"name": "Arduino Zero", "mcu": "SAMD21G18A"},
        (0x2341, 0x0241): {"name": "Arduino Uno WiFi Rev2", "mcu": "ATmega4809"},
        (0x10C4, 0xEA60): {"name": "ESP32 (CP210x)", "mcu": "ESP32"},
        (0x1A86, 0x7523): {"name": "Arduino (CH340)", "mcu": "ATmega328P/32U4"},
        (0x1A86, 0x55D4): {"name": "USB Serial (CH340K)", "mcu": "Unknown"},
        (0x0403, 0x6001): {"name": "FTDI FT232R", "mcu": "Unknown"},
        (0x0403, 0x6015): {"name": "FTDI FT231X", "mcu": "Unknown"},
        (0x2E8A, 0x000A): {"name": "Raspberry Pi Pico", "mcu": "RP2040"},
    }
    return boards.get((vid, pid))


def detect_boards() -> list[dict]:
    ports = serial.tools.list_ports.comports()
    boards = []
    for port in ports:
        # Filter out non-USB virtual consoles (ttyS*, ttyAMA*)
        dev = port.device or ""
        desc = (port.description or "").lower()
        if "ttyS" in dev and "usb" not in desc and port.vid is None:
            continue
        if "ttyAMA" in dev:
            continue
        info = {
            "port": port.device,
            "description": port.description,
            "manufacturer": port.manufacturer,
            "serial_number": port.serial_number,
            "hwid": port.hwid,
            "vid": None,
            "pid": None,
            "board_name": None,
            "mcu": None,
        }
        if port.vid and port.pid:
            info["vid"] = f"0x{port.vid:04X}"
            info["pid"] = f"0x{port.pid:04X}"
            board_info = get_board_info(port.vid, port.pid)
            if board_info:
                info["board_name"] = board_info["name"]
                info["mcu"] = board_info["mcu"]

        if not info["board_name"]:
            desc_lower = (port.description or "").lower()
            if "arduino" in desc_lower:
                info["board_name"] = port.description
            elif "cp210" in desc_lower or "ch340" in desc_lower:
                info["board_name"] = "USB Serial Device"

        boards.append(info)
    return boards


def get_arduino_cli_info() -> dict:
    try:
        version = subprocess.run(
            ["arduino-cli", "version", "--format", "json"],
            capture_output=True, text=True, timeout=5
        )
        return {"available": True, "version": json.loads(version.stdout)}
    except (FileNotFoundError, subprocess.TimeoutExpired, json.JSONDecodeError):
        return {"available": False, "version": None}
