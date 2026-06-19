/*
 * 3V 微型直流电机 — 单管驱动 + PWM 调速
 *
 * ============== 硬件接线 ==============
 *
 *  元件清单:
 *    NPN 三极管 1 个: S8050 / 2N2222 / BC547 (几毛钱)
 *    电阻 1 个: 1kΩ (棕黑红)
 *    二极管 1 个: 1N4148 / 1N4007 (保护用, 有就接)
 *    3V 电机 1 个
 *
 *  接线:
 *
 *      Arduino Uno           3V 电池或电源
 *      ┌─────────┐           ┌──────────┐
 *      │ D9 ───── 1kΩ ─── B (三极管基极)  │
 *      │ GND ──────────── E (三极管发射极) │
 *      │                      │           │
 *      │         3V (+) ──── 电机正极      │
 *      │         电机负极 ── C (三极管集电极)│
 *      │                      │           │
 *      │    (可选) 二极管反向并联在电机两端  │
 *      │    负极接 3V(+), 正极接 C          │
 *      └─────────┘           └──────────┘
 *
 *  供电选项:
 *    A) 电机 < 50mA → 用 Arduino 3.3V 引脚供电 (不用外接电源)
 *       3.3V ── 电机正极
 *    B) 电机 > 50mA → 外接 2 节 AA 电池 (3V)
 *       电池正极 ── 电机正极
 *       电池负极 ── Arduino GND (共地!)
 *
 *  S8050 引脚识别 (平面对自己, 脚朝下): 左=E 中=B 右=C
 *
 * ============== 串口指令 (9600bps) ==============
 *
 *  0-255 — 设置速度 (0=停, 255=全速)
 *  f     — 满速
 *  h     — 半速
 *  s     — 停止
 *  +     — 加速 10
 *  -     — 减速 10
 */

#define MOTOR_PIN 9

int speed = 128;  // 当前速度

void setup() {
  pinMode(MOTOR_PIN, OUTPUT);
  analogWrite(MOTOR_PIN, 0);
  Serial.begin(9600);
  Serial.println(F("3V DC Motor Driver (Transistor)"));
  Serial.println(F("  0-255=速度  f=满速  h=半速  s=停  +/-=微调"));
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() == 0) return;
    handleCmd(cmd);
  }
}

void handleCmd(String cmd) {
  if (cmd == "f" || cmd == "F") {
    speed = 255;
  } else if (cmd == "h" || cmd == "H") {
    speed = 128;
  } else if (cmd == "s" || cmd == "S") {
    speed = 0;
  } else if (cmd == "+" || cmd == "=") {
    speed = min(speed + 10, 255);
  } else if (cmd == "-" || cmd == "_") {
    speed = max(speed - 10, 0);
  } else {
    int val = cmd.toInt();
    if (val >= 0 && val <= 255) {
      speed = val;
    }
  }

  analogWrite(MOTOR_PIN, speed);
  int pct = map(speed, 0, 255, 0, 100);
  Serial.print(F("Speed: "));
  Serial.print(speed);
  Serial.print(F(" ("));
  Serial.print(pct);
  Serial.println(F("%)"));
}
