<template>
  <div class="board-detector">
    <div class="section-header">
      <h3>Device Explorer</h3>
      <button class="btn-refresh" @click="refresh" :disabled="loading">&#x21bb; Refresh</button>
    </div>

    <div v-if="loading" class="loading">Scanning ports...</div>

    <div v-else-if="error" class="error-box">
      <p>{{ error }}</p>
    </div>

    <div v-else-if="boards.length === 0" class="empty">
      <div class="empty-icon">&#128268;</div>
      <p>No devices detected</p>
      <p class="hint">Plug in your Arduino or dev board</p>
    </div>

    <div v-else class="boards-list">
      <div
        v-for="board in boards"
        :key="board.port"
        class="board-card"
        :class="{ selected: selectedPort === board.port }"
        @click="selectBoard(board)"
      >
        <div class="board-icon">&#128187;</div>
        <div class="board-info">
          <div class="board-name">{{ board.board_name || 'Unknown Device' }}</div>
          <div class="board-port">{{ board.port }}</div>
          <div class="board-meta">
            <span v-if="board.mcu" class="tag">{{ board.mcu }}</span>
            <span v-if="board.vid" class="tag">VID: {{ board.vid }}</span>
            <span v-if="board.pid" class="tag">PID: {{ board.pid }}</span>
          </div>
          <div v-if="board.description" class="board-desc">{{ board.description }}</div>
        </div>
      </div>
    </div>

    <div class="cli-status">
      <div v-if="arduinoCli.available" class="cli-ok">
        &#9989; arduino-cli v{{ arduinoCli.version?.Version || '?' }}
      </div>
      <div v-else class="cli-missing">
        <p>&#9888; arduino-cli not found</p>
        <p class="hint">Install for code compile/upload:</p>
        <code>curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh</code>
      </div>
    </div>
  </div>
</template>

<script>
import { getBoards } from '../api/index.js'

export default {
  name: 'BoardDetector',
  data() {
    return {
      boards: [],
      arduinoCli: { available: false },
      loading: false,
      error: null,
      selectedPort: null
    }
  },
  mounted() {
    this.refresh()
  },
  methods: {
    async refresh() {
      this.loading = true
      this.error = null
      try {
        const data = await getBoards()
        this.boards = data.boards || []
        this.arduinoCli = data.arduino_cli || { available: false }
      } catch (e) {
        this.error = 'Cannot connect to backend. Is it running?'
      } finally {
        this.loading = false
      }
    },
    selectBoard(board) {
      this.selectedPort = board.port
      this.$emit('select-board', { board, port: board.port })
    }
  }
}
</script>

<style scoped>
.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}
.section-header h3 {
  font-size: 14px;
  font-weight: 600;
  color: #e6edf3;
}
.btn-refresh {
  padding: 4px 10px;
  background: #21262d;
  color: #58a6ff;
  border: 1px solid #30363d;
  border-radius: 6px;
  font-size: 12px;
  cursor: pointer;
}
.btn-refresh:hover:not(:disabled) {
  background: #30363d;
}
.btn-refresh:disabled {
  opacity: 0.5;
}
.loading, .empty, .error-box {
  padding: 20px;
  text-align: center;
  color: #8b949e;
  font-size: 13px;
}
.empty-icon {
  font-size: 40px;
  margin-bottom: 10px;
}
.hint {
  font-size: 11px;
  color: #484f58;
  margin-top: 6px;
}
.error-box {
  color: #f85149;
  background: #1a0a0a;
  border: 1px solid #3a1111;
  border-radius: 6px;
}
.boards-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}
.board-card {
  display: flex;
  gap: 12px;
  padding: 12px;
  background: #0d1117;
  border: 1px solid #21262d;
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.15s;
}
.board-card:hover {
  border-color: #30363d;
  background: #161b22;
}
.board-card.selected {
  border-color: #58a6ff;
  background: #1a2233;
}
.board-icon {
  font-size: 28px;
  flex-shrink: 0;
}
.board-name {
  font-size: 13px;
  font-weight: 600;
  color: #e6edf3;
}
.board-port {
  font-size: 11px;
  color: #58a6ff;
  font-family: monospace;
  margin: 2px 0;
}
.board-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
  margin-top: 4px;
}
.tag {
  font-size: 10px;
  padding: 2px 6px;
  background: #21262d;
  border-radius: 4px;
  color: #8b949e;
  font-family: monospace;
}
.board-desc {
  font-size: 11px;
  color: #484f58;
  margin-top: 4px;
}
.cli-status {
  margin-top: 16px;
  padding: 10px;
  border-radius: 6px;
  font-size: 12px;
}
.cli-ok {
  color: #3fb950;
}
.cli-missing {
  background: #1a0f00;
  border: 1px solid #3a2200;
  padding: 10px;
  border-radius: 6px;
  color: #d29922;
}
.cli-missing code {
  display: block;
  margin-top: 6px;
  padding: 6px 8px;
  background: #0d1117;
  border-radius: 4px;
  font-size: 11px;
  color: #c9d1d9;
  word-break: break-all;
}
</style>
