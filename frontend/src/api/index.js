const BASE = '/api'

async function request(method, path, body) {
  const opts = {
    method,
    headers: { 'Content-Type': 'application/json' }
  }
  if (body) opts.body = JSON.stringify(body)
  const res = await fetch(`${BASE}${path}`, opts)
  return res.json()
}

export async function getBoards() {
  return request('GET', '/boards')
}

export async function connectSerial(port, baudrate = 9600) {
  return request('POST', '/serial/connect', { port, baudrate })
}

export async function disconnectSerial(port) {
  return request('POST', '/serial/disconnect', { port })
}

export async function writeSerial(port, data) {
  return request('POST', '/serial/write', { port, data })
}

export async function scanI2C(bus = 1) {
  return request('POST', '/i2c/scan', { bus })
}

export async function getI2CBuses() {
  return request('GET', '/i2c/buses')
}

export async function compileCode(code, board = 'arduino:avr:uno') {
  return request('POST', '/compile', { code, board })
}

export async function uploadCode(code, board = 'arduino:avr:uno', port = '') {
  return request('POST', '/upload', { code, board, port })
}

export async function getExamples() {
  return request('GET', '/examples')
}

export async function loadExample(name) {
  return request('GET', `/examples/${encodeURIComponent(name)}`)
}

export function createSerialSocket(port) {
  const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:'
  const name = port.split('/').pop()
  const wsUrl = `${protocol}//${location.host}/ws/serial/${name}`
  return new WebSocket(wsUrl)
}
