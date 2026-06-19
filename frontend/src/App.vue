<template>
  <div class="app">
    <header class="header">
      <div class="header-left">
        <span class="logo">&#9889; Smart Device Dev</span>
      </div>
      <div class="header-right">
        <span class="hint-key">Ctrl+/ Shortcuts</span>
        <span class="status-dot" :class="{ online: backendOnline }"></span>
        <span class="status-text">{{ backendOnline ? 'Backend OK' : 'Connecting...' }}</span>
      </div>
    </header>
    <main class="main">
      <aside class="sidebar" v-show="sidebarVisible">
        <BoardDetector @select-board="onSelectBoard" ref="boardDetector" />
      </aside>
      <section class="workspace">
        <div class="tabs">
          <button :class="{ active: activeTab === 'editor' }" @click="switchTab('editor')">
            Code Editor <kbd>^1</kbd>
          </button>
          <button :class="{ active: activeTab === 'monitor' }" @click="switchTab('monitor')">
            Serial Monitor <kbd>^2</kbd>
          </button>
          <button :class="{ active: activeTab === 'gpio' }" @click="switchTab('gpio')">
            GPIO Scanner <kbd>^3</kbd>
          </button>
        </div>
        <div class="tab-content">
          <CodeEditor
            v-if="activeTab === 'editor'"
            ref="codeEditor"
            :selected-board="selectedBoard"
            :selected-port="selectedPort"
            @focus-editor="onFocusEditor"
          />
          <SerialMonitor
            v-if="activeTab === 'monitor'"
            ref="serialMonitor"
            :selected-port="selectedPort"
            @focus-input="onFocusInput"
            @shift-tab-monitor="onShiftTab"
          />
          <GpioScanner
            v-if="activeTab === 'gpio'"
            ref="gpioScanner"
          />
        </div>
      </section>
    </main>

    <div v-if="showHelp" class="help-overlay" @click.self="showHelp = false">
      <div class="help-modal">
        <h3>Keyboard Shortcuts</h3>
        <table>
          <tbody>
          <tr><td><kbd>Ctrl</kbd> + <kbd>1</kbd></td><td>Code Editor</td></tr>
          <tr><td><kbd>Ctrl</kbd> + <kbd>2</kbd></td><td>Serial Monitor</td></tr>
          <tr><td><kbd>Ctrl</kbd> + <kbd>3</kbd></td><td>GPIO Scanner</td></tr>
          <tr><td><kbd>Ctrl</kbd> + <kbd>`</kbd></td><td>Toggle Sidebar</td></tr>
          <tr class="sep"><td colspan="2">Code Editor</td></tr>
          <tr><td><kbd>Ctrl</kbd> + <kbd>Enter</kbd></td><td>Verify / Compile</td></tr>
          <tr><td><kbd>Ctrl</kbd> + <kbd>U</kbd></td><td>Upload</td></tr>
          <tr><td><kbd>Ctrl</kbd> + <kbd>O</kbd></td><td>Focus Example Loader</td></tr>
          <tr><td><kbd>Esc</kbd></td><td>Close Build Output</td></tr>
          <tr class="sep"><td colspan="2">Serial Monitor</td></tr>
          <tr><td><kbd>Ctrl</kbd> + <kbd>K</kbd></td><td>Connect / Disconnect</td></tr>
          <tr><td><kbd>Ctrl</kbd> + <kbd>L</kbd></td><td>Clear Output</td></tr>
          <tr class="sep"><td colspan="2">GPIO Scanner</td></tr>
          <tr><td><kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>I</kbd></td><td>Scan I2C</td></tr>
          <tr class="sep"><td colspan="2">Global</td></tr>
          <tr><td><kbd>F5</kbd></td><td>Refresh Device List</td></tr>
          <tr><td><kbd>Ctrl</kbd> + <kbd>/</kbd></td><td>Show / Hide This Help</td></tr>
          </tbody>
        </table>
        <button class="btn-close-help" @click="showHelp = false">Close</button>
      </div>
    </div>
  </div>
</template>

<script>
import BoardDetector from './components/BoardDetector.vue'
import CodeEditor from './components/CodeEditor.vue'
import SerialMonitor from './components/SerialMonitor.vue'
import GpioScanner from './components/GpioScanner.vue'

export default {
  name: 'App',
  components: { BoardDetector, CodeEditor, SerialMonitor, GpioScanner },
  data() {
    return {
      activeTab: 'editor',
      selectedBoard: null,
      selectedPort: null,
      backendOnline: false,
      sidebarVisible: true,
      showHelp: false
    }
  },
  mounted() {
    this.checkBackend()
    setInterval(() => this.checkBackend(), 10000)
    document.addEventListener('keydown', this.onGlobalKey)
  },
  beforeUnmount() {
    document.removeEventListener('keydown', this.onGlobalKey)
  },
  methods: {
    async checkBackend() {
      try {
        const res = await fetch('/api/health')
        this.backendOnline = res.ok
      } catch {
        this.backendOnline = false
      }
    },
    onSelectBoard({ board, port }) {
      this.selectedBoard = board
      this.selectedPort = port
    },
    switchTab(tab) {
      this.activeTab = tab
      this.$nextTick(() => {
        if (tab === 'editor' && this.$refs.codeEditor) {
          this.$refs.codeEditor.$emit('focus-editor')
          this.$refs.codeEditor.focusEditor?.()
        }
        if (tab === 'monitor' && this.$refs.serialMonitor) {
          this.$refs.serialMonitor.focusInput?.()
        }
      })
    },
    onGlobalKey(e) {
      const ctrl = e.ctrlKey || e.metaKey
      const target = e.target

      // 不在 input/textarea/select 里（编辑器和串口输入自己处理）
      const inInput = target.tagName === 'INPUT' || target.tagName === 'TEXTAREA' || target.tagName === 'SELECT'

      // Ctrl+/ 切换帮助
      if (ctrl && e.key === '/') {
        e.preventDefault()
        this.showHelp = !this.showHelp
        return
      }

      // 全局 Tab 切换 (不在输入框)
      if (ctrl && !inInput) {
        if (e.key === '1') { e.preventDefault(); this.switchTab('editor'); return }
        if (e.key === '2') { e.preventDefault(); this.switchTab('monitor'); return }
        if (e.key === '3') { e.preventDefault(); this.switchTab('gpio'); return }
      }

      // 侧栏切换 Ctrl+`
      if (ctrl && e.key === '`') {
        e.preventDefault()
        this.sidebarVisible = !this.sidebarVisible
        return
      }

      // F5 刷新设备
      if (e.key === 'F5') {
        e.preventDefault()
        this.$refs.boardDetector?.refresh()
        return
      }

      // Esc 关闭帮助
      if (e.key === 'Escape' && this.showHelp) {
        this.showHelp = false
        e.preventDefault()
        return
      }

      // Serial Monitor shortcuts (even when input is focused)
      if (this.activeTab === 'monitor' && ctrl && e.key === 'k') {
        e.preventDefault()
        this.$refs.serialMonitor?.toggleConnection()
        return
      }
      if (this.activeTab === 'monitor' && ctrl && e.key === 'l') {
        e.preventDefault()
        this.$refs.serialMonitor?.clearOutput()
        return
      }

      // GPIO Scanner shortcut
      if (this.activeTab === 'gpio' && ctrl && e.shiftKey && e.key === 'I') {
        e.preventDefault()
        this.$refs.gpioScanner?.scan()
        return
      }
    },
    onFocusEditor() {
      this.activeTab = 'editor'
    },
    onFocusInput() {
      this.activeTab = 'monitor'
    },
    onShiftTab() {
      this.switchTab('editor')
    }
  }
}
</script>

<style>
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}
body {
  font-family: 'Inter', 'Segoe UI', system-ui, -apple-system, sans-serif;
  background: #0d1117;
  color: #c9d1d9;
  overflow: hidden;
  height: 100vh;
}
.app {
  display: flex;
  flex-direction: column;
  height: 100vh;
}
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 20px;
  height: 44px;
  background: #161b22;
  border-bottom: 1px solid #30363d;
  flex-shrink: 0;
}
.logo {
  font-size: 15px;
  font-weight: 700;
  color: #58a6ff;
}
.header-right {
  display: flex;
  align-items: center;
  gap: 10px;
  font-size: 12px;
  color: #8b949e;
}
.hint-key {
  color: #484f58;
  font-size: 11px;
}
.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: #f85149;
}
.status-dot.online {
  background: #3fb950;
}
.main {
  display: flex;
  flex: 1;
  overflow: hidden;
}
.sidebar {
  width: 300px;
  background: #161b22;
  border-right: 1px solid #30363d;
  overflow-y: auto;
  flex-shrink: 0;
  padding: 16px;
}
.workspace {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}
.tabs {
  display: flex;
  background: #161b22;
  border-bottom: 1px solid #30363d;
  padding: 0 16px;
  flex-shrink: 0;
}
.tabs button {
  padding: 10px 16px;
  background: none;
  border: none;
  color: #8b949e;
  font-size: 13px;
  cursor: pointer;
  border-bottom: 2px solid transparent;
  transition: all 0.15s;
  display: flex;
  align-items: center;
  gap: 6px;
}
.tabs button kbd {
  font-size: 10px;
  padding: 1px 5px;
  background: #21262d;
  border: 1px solid #30363d;
  border-radius: 3px;
  color: #484f58;
  font-family: monospace;
}
.tabs button.active kbd {
  color: #8b949e;
  border-color: #58a6ff33;
}
.tabs button:hover {
  color: #c9d1d9;
}
.tabs button.active {
  color: #58a6ff;
  border-bottom-color: #58a6ff;
}
.tab-content {
  flex: 1;
  overflow: hidden;
}

kbd {
  font-family: monospace;
}

.help-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0,0,0,0.6);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 100;
}
.help-modal {
  background: #161b22;
  border: 1px solid #30363d;
  border-radius: 12px;
  padding: 24px 28px;
  max-width: 480px;
  width: 90%;
}
.help-modal h3 {
  color: #e6edf3;
  font-size: 16px;
  margin-bottom: 16px;
}
.help-modal table {
  width: 100%;
  font-size: 13px;
  border-collapse: collapse;
}
.help-modal td {
  padding: 4px 8px;
  color: #8b949e;
}
.help-modal td:first-child {
  white-space: nowrap;
  padding-right: 20px;
}
.help-modal kbd {
  padding: 2px 6px;
  background: #21262d;
  border: 1px solid #30363d;
  border-radius: 4px;
  color: #c9d1d9;
  font-size: 12px;
}
.help-modal tr.sep td {
  padding-top: 10px;
  color: #58a6ff;
  font-weight: 600;
  font-size: 11px;
}
.btn-close-help {
  margin-top: 16px;
  padding: 6px 18px;
  background: #21262d;
  color: #c9d1d9;
  border: 1px solid #30363d;
  border-radius: 6px;
  font-size: 13px;
  cursor: pointer;
}
.btn-close-help:hover {
  background: #30363d;
}

::-webkit-scrollbar {
  width: 8px;
}
::-webkit-scrollbar-track {
  background: #0d1117;
}
::-webkit-scrollbar-thumb {
  background: #30363d;
  border-radius: 4px;
}
::-webkit-scrollbar-thumb:hover {
  background: #484f58;
}
</style>
