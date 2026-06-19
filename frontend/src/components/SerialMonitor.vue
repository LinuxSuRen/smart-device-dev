<template>
  <div class="monitor-container">
    <div class="monitor-toolbar">
      <div class="toolbar-left">
        <span v-if="selectedPort" class="port-label">Port: <code>{{ selectedPort }}</code></span>
        <span v-else class="port-label no-port">No device selected</span>
      </div>
      <div class="toolbar-right">
        <select v-model.number="baudrate" class="btn-select">
          <option :value="300">300</option>
          <option :value="1200">1200</option>
          <option :value="2400">2400</option>
          <option :value="4800">4800</option>
          <option :value="9600">9600</option>
          <option :value="19200">19200</option>
          <option :value="38400">38400</option>
          <option :value="57600">57600</option>
          <option :value="115200">115200</option>
          <option :value="230400">230400</option>
          <option :value="460800">460800</option>
          <option :value="921600">921600</option>
        </select>
        <button
          class="btn-action"
          :class="{ connected }"
          @click="toggleConnection"
          :disabled="!selectedPort"
        >
          {{ connected ? 'Disconnect' : 'Connect' }} <kbd>^K</kbd>
        </button>
        <button class="btn-action" @click="clearOutput">
          Clear <kbd>^L</kbd>
        </button>
        <label class="chk-autoscroll">
          <input type="checkbox" v-model="autoscroll" /> Auto-scroll
        </label>
      </div>
    </div>

    <div ref="outputEl" class="monitor-output" tabindex="-1"
         @keydown.esc="focusInputIfConnected">
      <div v-if="!connected" class="monitor-placeholder">
        <p>&#128225; Connect to a serial port to see output</p>
        <p class="hint" v-if="!selectedPort">Select a device from the sidebar first</p>
      </div>
      <pre v-else class="output-text">{{ output }}</pre>
    </div>

    <div class="monitor-input">
      <input
        ref="inputEl"
        type="text"
        v-model="inputText"
        placeholder="Type to send..."
        @keyup.enter="sendInput"
        :disabled="!connected"
        @focus="$emit('focus-input')"
      />
      <button @click="sendInput" :disabled="!connected" class="btn-send">Send</button>
      <label class="chk-newline">
        <input type="checkbox" v-model="addNewline" /> \n
      </label>
      <label class="chk-newline">
        <input type="checkbox" v-model="addCarriage" /> \r
      </label>
    </div>
  </div>
</template>

<script>
import { connectSerial, disconnectSerial, createSerialSocket } from '../api/index.js'

export default {
  name: 'SerialMonitor',
  props: {
    selectedPort: String
  },
  emits: ['focus-input'],
  data() {
    return {
      baudrate: 9600,
      connected: false,
      output: '',
      inputText: '',
      addNewline: true,
      addCarriage: false,
      autoscroll: true,
      ws: null
    }
  },
  watch: {
    selectedPort() {
      if (this.connected) {
        this.disconnect()
      }
    }
  },
  beforeUnmount() {
    this.disconnect()
  },
  methods: {
    focusInput() {
      this.$nextTick(() => {
        this.$refs.inputEl?.focus()
      })
    },
    focusInputIfConnected() {
      if (this.connected) this.focusInput()
    },
    async toggleConnection() {
      if (this.connected) {
        this.disconnect()
      } else {
        await this.connect()
        this.$nextTick(() => this.$refs.inputEl?.focus())
      }
    },
    async connect() {
      try {
        const res = await connectSerial(this.selectedPort, this.baudrate)
        if (res.success) {
          this.connected = true
          this.openWebSocket()
        }
      } catch {
        this.output += '[Error] Cannot connect to backend\n'
      }
    },
    disconnect() {
      this.connected = false
      if (this.ws) {
        this.ws.close()
        this.ws = null
      }
      if (this.selectedPort) {
        disconnectSerial(this.selectedPort).catch(() => {})
      }
    },
    openWebSocket() {
      if (this.ws) this.ws.close()
      const ws = createSerialSocket(this.selectedPort)
      ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data)
          if (msg.type === 'data') {
            this.output += msg.data
            if (this.autoscroll) {
              this.$nextTick(() => {
                const el = this.$refs.outputEl
                if (el) el.scrollTop = el.scrollHeight
              })
            }
          }
        } catch {}
      }
      ws.onclose = () => {
        this.output += '\n[Disconnected]\n'
        this.connected = false
      }
      ws.onerror = () => {
        this.output += '\n[Connection Error]\n'
        this.connected = false
      }
      this.ws = ws
    },
    sendInput() {
      if (!this.inputText || !this.ws) return
      let data = this.inputText
      if (this.addCarriage) data += '\r'
      if (this.addNewline) data += '\n'
      this.ws.send(data)
      const display = data.replace(/\r/g, '\\r').replace(/\n/g, '\\n')
      this.output += `> ${display}\n`
      this.inputText = ''
    },
    clearOutput() {
      this.output = ''
    }
  }
}
</script>

<style scoped>
.monitor-container {
  display: flex;
  flex-direction: column;
  height: 100%;
}
.monitor-toolbar {
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
.toolbar-left, .toolbar-right {
  display: flex;
  align-items: center;
  gap: 8px;
}
.port-label {
  font-size: 12px;
  color: #8b949e;
}
.port-label code {
  color: #58a6ff;
}
.port-label.no-port {
  color: #d29922;
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
.btn-action.connected {
  background: #da3633;
  border-color: #f85149;
  color: #fff;
}
.btn-action.connected:hover:not(:disabled) {
  background: #f85149;
}
.chk-autoscroll, .chk-newline {
  font-size: 12px;
  color: #8b949e;
  display: flex;
  align-items: center;
  gap: 4px;
}
.monitor-output {
  flex: 1;
  overflow-y: auto;
  background: #0d1117;
  padding: 12px;
}
.monitor-placeholder {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 100%;
  color: #484f58;
  font-size: 14px;
}
.hint {
  font-size: 12px;
  margin-top: 8px;
  color: #30363d;
}
.output-text {
  font-family: 'Fira Code', 'Consolas', monospace;
  font-size: 13px;
  line-height: 1.6;
  white-space: pre-wrap;
  word-break: break-all;
  color: #c9d1d9;
  margin: 0;
}
.monitor-input {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  background: #161b22;
  border-top: 1px solid #30363d;
  flex-shrink: 0;
}
.monitor-input input {
  flex: 1;
  padding: 6px 10px;
  background: #0d1117;
  color: #c9d1d9;
  border: 1px solid #30363d;
  border-radius: 6px;
  font-size: 13px;
  font-family: monospace;
}
.monitor-input input:focus {
  outline: none;
  border-color: #58a6ff;
}
.monitor-input input:disabled {
  opacity: 0.4;
}
.btn-send {
  padding: 6px 16px;
  background: #21262d;
  color: #c9d1d9;
  border: 1px solid #30363d;
  border-radius: 6px;
  font-size: 12px;
  cursor: pointer;
}
.btn-send:hover:not(:disabled) {
  background: #30363d;
}
.btn-send:disabled {
  opacity: 0.4;
}
</style>
