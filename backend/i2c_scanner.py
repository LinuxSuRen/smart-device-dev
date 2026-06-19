import subprocess
import os
import platform
from typing import Optional

# Known I2C device addresses
I2C_DEVICES = {
    0x03: "PCA9536 4-bit I/O",
    0x0E: "MAG3110 Magnetometer",
    0x0F: "BMP180 Pressure Sensor (alt)",
    0x10: "VEML6075 UV Sensor",
    0x11: "Si1145 Light/IR Sensor",
    0x12: "ADXL345 Accelerometer (alt)",
    0x13: "VCNL40x0 Proximity Sensor",
    0x15: "BH1750FVI Ambient Light (alt)",
    0x18: "MCP9808 Temperature Sensor / LIS3DH Accel",
    0x19: "LIS3DH Accelerometer (alt)",
    0x1A: "ADXL345 Accelerometer (alt)",
    0x1C: "LIS3MDL Magnetometer / MMA845x Accel",
    0x1D: "LSM9DS1 Accel+Mag / ADXL345",
    0x1E: "LSM303D/HMC5883L/LIS3MDL Magnetometer",
    0x20: "MCP23017 GPIO Expander / PCF8574",
    0x21: "PCF8574 / MCP23017 (alt)",
    0x22: "PCF8574A / MCP23017 (alt)",
    0x23: "BH1750FVI Ambient Light Sensor",
    0x24: "PCF8574 / MCP23017 (alt)",
    0x25: "PCF8574A (alt)",
    0x26: "PCF8574A (alt)",
    0x27: "PCF8574 / LCD I2C Backpack",
    0x28: "BMP280 Pressure/Temp (alt) / TSL2561",
    0x29: "TSL2561 Light Sensor / VL6180X ToF",
    0x2A: "CAP1188 Capacitive Touch",
    0x2C: "INA219 Current Sensor (alt)",
    0x2D: "INA219 Current Sensor (alt)",
    0x30: "Si7021 Temp/Humidity (alt)",
    0x33: "MAX17048 Battery Fuel Gauge",
    0x34: "MAX17048 (alt) / AXP192 PMIC",
    0x35: "MAX17048 (alt) / AXP192 PMIC (alt)",
    0x36: "MAX17048 (alt) / AXP192 PMIC (alt)",
    0x38: "VEML6070 UV Sensor / AHT10 Temp/Hum",
    0x39: "TSL2561 (alt) / APDS-9960 Gesture",
    0x3A: "SSD1306 OLED (alt)",
    0x3C: "SSD1306 OLED Display 128x64",
    0x3D: "SSD1306 OLED Display 128x64 (alt)",
    0x3E: "SSD1306 OLED Display (alt)",
    0x3F: "PCF8574 LCD I2C Backpack (alt)",
    0x40: "INA219 / PCA9685 PWM Driver / Si7021",
    0x41: "INA219 (alt) / TMP007 IR Thermopile",
    0x44: "SHT31-D Temp/Humidity / OPT3001 Light",
    0x45: "SHT31-D (alt)",
    0x48: "ADS1115 16-bit ADC / TMP102 Temp",
    0x49: "ADS1115 (alt) / TMP102 (alt)",
    0x4A: "ADS1115 (alt) / TMP102 (alt)",
    0x4B: "TMP102 (alt) / ADS1115 (alt)",
    0x4C: "TMP102 (alt) / SHT3x-DIS",
    0x4D: "TMP102 (alt) / SHT3x-DIS (alt)",
    0x50: "AT24C32 EEPROM / PCF8563 RTC",
    0x51: "PCF8563 RTC (alt) / MCP794xx RTC",
    0x52: "Nintendo Nunchuk / PCF8563 (alt)",
    0x53: "ADXL345 Accel / LSM6DSO",
    0x54: "BME688 Gas/Temp/Pressure/Hum",
    0x55: "BME688 (alt) / MAX3010x PulseOx",
    0x56: "BME688 (alt)",
    0x57: "BME688 (alt) / MAX3010x (alt)",
    0x58: "SGP30 Gas Sensor / TCS3472 Color",
    0x59: "SGP30 (alt) / TCS3472 (alt)",
    0x5A: "MLX90614 IR Thermometer / CCS811 VOC",
    0x5B: "MLX90614 (alt) / CCS811 (alt)",
    0x5C: "AM2320 Temp/Hum / LSM6DS3 Accel+Gyro",
    0x5D: "AM2320 (alt) / LSM6DS3 (alt)",
    0x60: "MPL3115A2 Pressure / Si5351A Clock",
    0x61: "SCD30 CO2 Sensor / MPL3115A2 (alt)",
    0x62: "SCD30 CO2 (alt)",
    0x63: "SCD30 CO2 (alt) / MMA8451 Accel",
    0x64: "LPS25HB/MPL3115A2 (alt)",
    0x68: "DS3231 RTC / MPU-6050/MPU-9250 IMU",
    0x69: "DS3231 (alt) / MPU-6050/9250 (alt)",
    0x6A: "MPU-6050 (alt) / LSM303AGR",
    0x6B: "MPU-6050 (alt) / LSM303AGR (alt)",
    0x6C: "MPU-6050 (alt)",
    0x6D: "BME280 (alt)",
    0x6E: "BME280 (alt)",
    0x6F: "BME280 Pressure/Temp/Hum (alt)",
    0x70: "HT16K33 LED Driver / TCA9548A Mux",
    0x71: "HT16K33 (alt) / TCA9548A (alt)",
    0x72: "HT16K33 (alt) / TCA9548A (alt)",
    0x73: "HT16K33 (alt) / TCA9548A (alt)",
    0x74: "TCA9548A I2C Mux (alt)",
    0x75: "TCA9548A (alt)",
    0x76: "BME280/BMP280 Pressure/Temp/Hum",
    0x77: "BME280/BMP280 (alt) / BMA180 Accel",
    0x78: "OLED SSD1306 (alt)",
    0x7A: "OLED SSD1306 (alt)",
    0x7C: "OLED SSD1306 (alt) / PCF8574 (alt)",
}


def detect_chip_info(address: int) -> str:
    return I2C_DEVICES.get(address, "Unknown Device")


def scan_i2c_bus(bus: int = 1) -> list[dict]:
    system = platform.system()
    if system == "Linux":
        return _scan_i2c_linux(bus)
    else:
        return [{"status": "error", "message": "I2C scanning is only supported on Linux"}]


def _scan_i2c_linux(bus: int) -> list[dict]:
    i2c_device = f"/dev/i2c-{bus}"
    if not os.path.exists(i2c_device):
        return [{"status": "error", "message": f"I2C bus {bus} not available. Check if i2c-dev module is loaded."}]

    try:
        result = subprocess.run(
            ["i2cdetect", "-y", "-r", str(bus)],
            capture_output=True, text=True, timeout=10
        )
        if result.returncode != 0:
            return [{"status": "error", "message": f"i2cdetect failed: {result.stderr.strip()}"}]

        devices = []
        for line in result.stdout.split("\n"):
            if ":" in line and not line.startswith(" "):
                parts = line.split()
                for part in parts[1:]:
                    if part != "--" and len(part) == 2:
                        try:
                            addr = int(part, 16)
                            devices.append({
                                "address": f"0x{addr:02X}",
                                "address_dec": addr,
                                "chip": detect_chip_info(addr)
                            })
                        except ValueError:
                            pass
        return devices if devices else [{"status": "info", "message": "No I2C devices found on the bus"}]
    except FileNotFoundError:
        return [{"status": "error", "message": "i2cdetect not found. Install i2c-tools: sudo apt install i2c-tools"}]
    except Exception as e:
        return [{"status": "error", "message": str(e)}]


def list_i2c_buses() -> list[str]:
    buses = []
    system = platform.system()
    if system == "Linux":
        for i in range(0, 8):
            if os.path.exists(f"/dev/i2c-{i}"):
                buses.append(f"/dev/i2c-{i}")
    return buses
