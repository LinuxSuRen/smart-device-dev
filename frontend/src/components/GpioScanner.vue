<template>
  <div class="gpio-container">
    <div class="gpio-toolbar">
      <div class="toolbar-left">
        <h3>I2C Bus Scanner</h3>
      </div>
      <div class="toolbar-right">
        <span class="bus-label">Bus:</span>
        <select v-model.number="selectedBus" class="btn-select">
          <option v-for="b in buses" :key="b" :value="b">{{ b }}</option>
          <option v-if="buses.length === 0" value="1">i2c-1</option>
        </select>
        <button class="btn-action primary" @click="scan" :disabled="scanning">
          {{ scanning ? 'Scanning...' : 'Scan I2C' }} <kbd>^Shift+I</kbd>
        </button>
        <button class="btn-action" @click="refreshBuses">Refresh Buses</button>
      </div>
    </div>

    <div class="gpio-content">
      <div v-if="error" class="error-msg">{{ error }}</div>
      <div v-else-if="devices.length === 0 && !scanning && hasScanned" class="empty-msg">
        No I2C devices found on bus
      </div>
      <div v-else-if="!hasScanned" class="placeholder">
        <p>&#128300; Click "Scan I2C" to detect devices on the bus</p>
      </div>

      <div v-if="devices.length > 0" class="devices-grid">
        <div v-for="device in devices" :key="device.address" class="device-card">
          <div class="device-addr">{{ device.address }}</div>
          <div class="device-chip">{{ device.chip }}</div>
          <div class="device-decimal">dec: {{ device.address_dec }}</div>
        </div>
      </div>
    </div>

    <div class="gpio-info">
      <details>
        <summary>About I2C / GPIO Scanning</summary>
        <div class="info-content">
          <p>This tool scans the I2C bus to detect connected chips and sensors.</p>
          <p><strong>Requirements:</strong></p>
          <ul>
            <li>Linux system with <code>i2c-tools</code> installed</li>
            <li>I2C kernel module loaded: <code>sudo modprobe i2c-dev</code></li>
            <li>Proper GPIO/I2C wiring between the board and chips</li>
          </ul>
          <p><strong>Install i2c-tools:</strong></p>
          <code>sudo apt install i2c-tools</code>
          <p style="margin-top:8px"><strong>Enable I2C on Raspberry Pi:</strong></p>
          <code>sudo raspi-config</code> → Interface Options → I2C → Enable
        </div>
      </details>
    </div>
  </div>
</template>

<script>
import { scanI2C, getI2CBuses } from '../api/index.js'

export default {
  name: 'GpioScanner',
  data() {
    return {
      buses: [],
      selectedBus: 1,
      devices: [],
      scanning: false,
      hasScanned: false,
      error: null
    }
  },
  mounted() {
    this.refreshBuses()
  },
  methods: {
    async refreshBuses() {
      try {
        const data = await getI2CBuses()
        this.buses = data.buses
          ? data.buses.map(b => parseInt(b.match(/\d+$/)?.[0] || 0)).filter(n => !isNaN(n))
          : []
        if (this.buses.length === 0) this.buses = [1]
      } catch {
        this.buses = [1]
      }
    },
    async scan() {
      this.scanning = true
      this.error = null
      this.devices = []
      try {
        const data = await scanI2C(this.selectedBus)
        this.hasScanned = true
        if (data.devices) {
          const first = data.devices[0]
          if (first?.status === 'error') {
            this.error = first.message
          } else if (first?.status) {
            this.devices = []
          } else {
            this.devices = data.devices
          }
        }
      } catch {
        this.error = 'Failed to connect to backend'
      } finally {
        this.scanning = false
      }
    }
  }
}
</script>

<style scoped>
.gpio-container {
  display: flex;
  flex-direction: column;
  height: 100%;
}
.gpio-toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 12px;
  background: #161b22;
  border-bottom: 1px solid #30363d;
  flex-shrink: 0;
  flex-wrap: wrap;
  gap: 8px;
}
.toolbar-left h3 {
  font-size: 14px;
  font-weight: 600;
  color: #e6edf3;
  margin: 0;
}
.toolbar-right {
  display: flex;
  align-items: center;
  gap: 8px;
}
.bus-label {
  font-size: 12px;
  color: #8b949e;
}
.btn-select {
  padding: 4px 8px;
  background: #21262d;
  color: #c9d1d9;
  border: 1px solid #30363d;
  border-radius: 6px;
  font-size: 12px;
}
.btn-action {
  padding: 5px 12px;
  background: #21262d;
  color: #c9d1d9;
  border: 1px solid #30363d;
  border-radius: 6px;
  font-size: 12px;
  cursor: pointer;
}
.btn-action:hover:not(:disabled) {
  background: #30363d;
}
.btn-action:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}
.btn-action kbd {
  font-size: 9px;
  padding: 1px 4px;
  background: #30363d;
  border-radius: 3px;
  margin-left: 4px;
  opacity: 0.6;
}
.btn-action.primary {
  background: #238636;
  color: #fff;
  border-color: #2ea043;
}
.btn-action.primary:hover:not(:disabled) {
  background: #2ea043;
}
.btn-action.primary kbd {
  background: #1b5a2a;
}
.gpio-content {
  flex: 1;
  overflow-y: auto;
  padding: 16px;
}
.placeholder, .empty-msg, .error-msg {
  text-align: center;
  padding: 40px 20px;
  color: #484f58;
  font-size: 14px;
}
.error-msg {
  color: #f85149;
  background: #1a0a0a;
  border: 1px solid #3a1111;
  border-radius: 6px;
}
.devices-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
  gap: 10px;
}
.device-card {
  padding: 12px;
  background: #0d1117;
  border: 1px solid #21262d;
  border-radius: 8px;
  transition: border-color 0.15s;
}
.device-card:hover {
  border-color: #58a6ff;
}
.device-addr {
  font-family: 'Fira Code', monospace;
  font-size: 16px;
  font-weight: 700;
  color: #58a6ff;
}
.device-chip {
  font-size: 13px;
  color: #e6edf3;
  margin: 6px 0 2px;
  line-height: 1.3;
}
.device-decimal {
  font-size: 11px;
  color: #484f58;
  font-family: monospace;
}
.gpio-info {
  padding: 8px 12px;
  background: #161b22;
  border-top: 1px solid #30363d;
  flex-shrink: 0;
}
.gpio-info summary {
  font-size: 12px;
  color: #8b949e;
  cursor: pointer;
}
.gpio-info summary:hover {
  color: #c9d1d9;
}
.info-content {
  margin-top: 8px;
  padding: 10px;
  background: #0d1117;
  border-radius: 6px;
  font-size: 12px;
  color: #8b949e;
  line-height: 1.6;
}
.info-content p {
  margin: 4px 0;
}
.info-content code {
  padding: 2px 6px;
  background: #21262d;
  border-radius: 3px;
  font-size: 11px;
}
.info-content ul {
  padding-left: 20px;
  margin: 6px 0;
}
.info-content li {
  margin: 3px 0;
}
</style>
