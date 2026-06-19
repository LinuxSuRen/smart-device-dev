/*
 * L9110S 单风扇调速 (2 线电机, 单向)
 *
 * ============== 硬件接线 (仅 3 根线) ==============
 *
 *  L9110S 模块        Arduino Uno
 *  ┌──────────┐      ┌──────────┐
 *  │ VCC ──────────── 5V        │
 *  │ GND ──────────── GND       │
 *  │ A-IA ─────────── D9        │
 *  │ A-IB ─── GND (模块上短接)   │
 *  │                             │
 *  │ MOTOR A ── 风扇正极          │
 *  │ MOTOR A ── 风扇负极          │
 *  └──────────┘      └──────────┘
 *
 *  L9110S 淘宝 3 元包邮. 体积 < 指甲盖.
 *
 * ============== 串口指令 (9600bps) ==============
 *
 *  0-255 — 速度 (0=停, 255=全速)
 *  f     — 全速
 *  h     — 半速
 *  s     — 停
 *  + / - — 微调±10
 */

#define FAN_PIN 9

int speed = 128;

void setup() {
  pinMode(FAN_PIN, OUTPUT);
  analogWrite(FAN_PIN, 0);

  Serial.begin(9600);
  Serial.println(F("L9110S Fan Driver"));
  Serial.println(F("  0-255=速度  f=满速  h=半速  s=停  +/-=微调"));
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() == 0) return;

    if (cmd == "f" || cmd == "F")      speed = 255;
    else if (cmd == "h" || cmd == "H") speed = 128;
    else if (cmd == "s" || cmd == "S") speed = 0;
    else if (cmd == "+" || cmd == "=") speed = min(speed + 10, 255);
    else if (cmd == "-" || cmd == "_") speed = max(speed - 10, 0);
    else {
      int val = cmd.toInt();
      if (val >= 0 && val <= 255) speed = val;
    }

    analogWrite(FAN_PIN, speed);
    Serial.print(F("Speed: "));
    Serial.print(speed);
    Serial.print(F(" ("));
    Serial.print(map(speed, 0, 255, 0, 100));
    Serial.println(F("%)"));
  }
}
