<template>
  <div class="editor-container">
    <div class="editor-toolbar">
      <select v-model="boardFqbn" class="btn-select" ref="fqbnSelect">
        <option value="arduino:avr:uno">Arduino Uno</option>
        <option value="arduino:avr:mega">Arduino Mega</option>
        <option value="arduino:avr:nano">Arduino Nano</option>
        <option value="arduino:avr:leonardo">Arduino Leonardo</option>
        <option value="arduino:sam:arduino_due_x">Arduino Due</option>
        <option value="arduino:samd:arduino_zero_native">Arduino Zero</option>
        <option value="esp32:esp32:esp32">ESP32</option>
      </select>
      <select v-model="selectedExample" class="btn-select" @change="loadExample" ref="exampleSelect">
        <option value="">-- Load Example --</option>
        <option v-for="ex in examples" :key="ex.name" :value="ex.name">{{ ex.name }}</option>
      </select>
      <div class="toolbar-spacer"></div>
      <button class="btn-action" @click="compile" :disabled="compiling || uploading">
        {{ compiling ? 'Compiling...' : 'Verify' }} <kbd>^Enter</kbd>
      </button>
      <button
        class="btn-action btn-compile-upload"
        @click="compileAndUpload"
        :disabled="compiling || uploading || !selectedPort"
      >
        {{ compiling ? 'Compiling...' : uploading ? 'Uploading...' : 'Compile & Upload' }} <kbd>^S U</kbd>
      </button>
      <button class="btn-action primary" @click="upload" :disabled="compiling || uploading || !selectedPort">
        {{ uploading ? 'Uploading...' : 'Upload' }} <kbd>^U</kbd>
      </button>
    </div>

    <div ref="editorEl" class="editor-pane"></div>

    <div v-if="output" class="output-pane">
      <div class="output-header">
        <span>{{ output.success ? 'Build OK' : 'Build Failed' }}</span>
        <button class="btn-close" @click="output = null">Esc</button>
      </div>
      <pre class="output-content" :class="{ 'output-error': !output.success }">{{ output.text }}</pre>
    </div>
  </div>
</template>

<script>
import { basicSetup } from 'codemirror'
import { EditorView, keymap } from '@codemirror/view'
import { EditorState } from '@codemirror/state'
import { cpp } from '@codemirror/lang-cpp'
import { oneDark } from '@codemirror/theme-one-dark'
import { indentWithTab } from '@codemirror/commands'
import { compileCode, uploadCode, getExamples, loadExample as fetchExample } from '../api/index.js'

const DEFAULT_CODE = `void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Hello from Arduino!");
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}
`

export default {
  name: 'CodeEditor',
  props: {
    selectedBoard: Object,
    selectedPort: String
  },
  emits: ['focus-editor'],
  data() {
    return {
      editor: null,
      boardFqbn: localStorage.getItem('sd_boardFqbn') || 'arduino:avr:uno',
      compiling: false,
      uploading: false,
      output: null,
      examples: [],
      selectedExample: localStorage.getItem('sd_selectedExample') || ''
    }
  },
  mounted() {
    this.initEditor()
    this.fetchExamples()
    this._onKey = (e) => {
      const ctrl = e.ctrlKey || e.metaKey
      if (ctrl && e.key === 'o') {
        e.preventDefault()
        this.$refs.exampleSelect?.focus()
      }
      if (e.key === 'Escape' && this.output) {
        e.preventDefault()
        this.output = null
      }
    }
    document.addEventListener('keydown', this._onKey)
  },
  beforeUnmount() {
    if (this.editor) this.editor.destroy()
    document.removeEventListener('keydown', this._onKey)
  },
  watch: {
    selectedBoard(board) {
      if (board?.board_name) {
        const map = {
          'Arduino Uno': 'arduino:avr:uno',
          'Arduino Mega': 'arduino:avr:mega:cpu=atmega2560',
          'Arduino Nano': 'arduino:avr:nano',
          'Arduino Leonardo': 'arduino:avr:leonardo',
          'Arduino Due': 'arduino:sam:arduino_due_x',
          'Arduino Zero': 'arduino:samd:arduino_zero_native'
        }
        this.boardFqbn = map[board.board_name] || this.boardFqbn
      }
    },
    boardFqbn(val) {
      localStorage.setItem('sd_boardFqbn', val)
    },
    selectedExample(val) {
      localStorage.setItem('sd_selectedExample', val)
    }
  },
  methods: {
    initEditor() {
      const vm = this
      const state = EditorState.create({
        doc: DEFAULT_CODE,
        extensions: [
          basicSetup,
          cpp(),
          oneDark,
          keymap.of([
            indentWithTab,
            {
              key: 'Ctrl-Enter',
              run() { vm.compile(); return true }
            },
            {
              key: 'Ctrl-u',
              run() { vm.upload(); return true }
            },
            {
              key: 'Ctrl-Shift-u',
              run() { vm.compileAndUpload(); return true }
            }
          ]),
          EditorView.updateListener.of(() => {
            this.$emit('code-change', this.getCode())
          }),
          EditorView.theme({
            '&': { height: '100%' },
            '.cm-editor': { height: '100%' },
            '.cm-scroller': { overflow: 'auto' }
          })
        ]
      })
      this.editor = new EditorView({
        state,
        parent: this.$refs.editorEl
      })

      this.editor.dom.addEventListener('focus', () => {
        this.$emit('focus-editor')
      })
    },
    focusEditor() {
      this.editor?.focus()
    },
    getCode() {
      return this.editor?.state.doc.toString() || ''
    },
    async fetchExamples() {
      try {
        const res = await getExamples()
        this.examples = res.examples || []
        if (this.selectedExample && this.examples.find(e => e.name === this.selectedExample)) {
          this.loadExample()
        }
      } catch {
        this.examples = []
      }
    },
    async loadExample() {
      if (!this.selectedExample || !this.editor) return
      try {
        const res = await fetchExample(this.selectedExample)
        if (res.success && res.content) {
          const view = this.editor
          view.dispatch({
            changes: { from: 0, to: view.state.doc.length, insert: res.content }
          })
        }
      } catch {
        // silently ignore
      }
    },
    async compile() {
      this.compiling = true
      this.output = null
      try {
        const res = await compileCode(this.getCode(), this.boardFqbn)
        this.output = { success: res.success, text: res.success ? res.output : (res.error || res.output) }
      } catch (e) {
        this.output = { success: false, text: 'Failed to connect to backend' }
      } finally {
        this.compiling = false
      }
    },
    async upload() {
      this.uploading = true
      this.output = null
      try {
        const res = await uploadCode(this.getCode(), this.boardFqbn, this.selectedPort)
        this.output = { success: res.success, text: res.success ? res.output : (res.error || res.output) }
      } catch (e) {
        this.output = { success: false, text: 'Failed to connect to backend' }
      } finally {
        this.uploading = false
      }
    },
    async compileAndUpload() {
      this.compiling = true
      this.uploading = false
      this.output = null
      try {
        const compileRes = await compileCode(this.getCode(), this.boardFqbn)
        if (!compileRes.success) {
          this.output = { success: false, text: compileRes.error || compileRes.output }
          this.compiling = false
          return
        }
        this.output = { success: true, text: 'Compile OK. Uploading...' }
        this.compiling = false
        this.uploading = true
        const uploadRes = await uploadCode(this.getCode(), this.boardFqbn, this.selectedPort)
        this.output = { success: uploadRes.success, text: uploadRes.success ? uploadRes.output : (uploadRes.error || uploadRes.output) }
      } catch (e) {
        this.output = { success: false, text: 'Failed to connect to backend' }
      } finally {
        this.uploading = false
        this.compiling = false
      }
    }
  }
}
</script>

<style scoped>
.editor-container {
  display: flex;
  flex-direction: column;
  height: 100%;
}
.editor-toolbar {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  background: #161b22;
  border-bottom: 1px solid #30363d;
  flex-shrink: 0;
}
.btn-select {
  padding: 5px 10px;
  background: #21262d;
  color: #c9d1d9;
  border: 1px solid #30363d;
  border-radius: 6px;
  font-size: 12px;
}
.toolbar-spacer {
  flex: 1;
}
.btn-action {
  padding: 5px 14px;
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
.btn-compile-upload {
  background: #1f6feb;
  color: #fff;
  border-color: #388bfd;
}
.btn-compile-upload:hover:not(:disabled) {
  background: #388bfd;
}
.btn-compile-upload kbd {
  background: #1c4d9e;
}
.editor-pane {
  flex: 1;
  overflow: hidden;
}
.output-pane {
  flex-shrink: 0;
  max-height: 200px;
  overflow-y: auto;
  background: #0d1117;
  border-top: 1px solid #30363d;
}
.output-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 6px 12px;
  font-size: 12px;
  color: #8b949e;
  background: #161b22;
  border-bottom: 1px solid #30363d;
}
.btn-close {
  background: none;
  border: none;
  color: #484f58;
  font-size: 11px;
  cursor: pointer;
}
.btn-close:hover {
  color: #c9d1d9;
}
.output-content {
  padding: 10px 12px;
  font-family: 'Fira Code', 'Consolas', monospace;
  font-size: 12px;
  line-height: 1.5;
  white-space: pre-wrap;
  word-break: break-all;
  color: #c9d1d9;
  margin: 0;
}
.output-error {
  color: #f85149;
}
</style>
