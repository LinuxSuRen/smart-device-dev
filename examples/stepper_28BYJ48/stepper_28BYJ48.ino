/*
 * 28BYJ-48 步进电机 + ULN2003 驱动板
 *
 * ============== 硬件接线 ==============
 *
 *  ULN2003 驱动板  ->  Arduino Uno    28BYJ-48 电机 -> ULN2003
 *  -------------------------------    ---------------------
 *  IN1            -> D2               5P 排线直插驱动板插座
 *  IN2            -> D3
 *  IN3            -> D4
 *  IN4            -> D5               注意: 排线蓝色线对 IN1 侧
 *  VCC (+)        -> 5V
 *  GND (-)        -> GND
 *
 *  ⚠ 驱动板跳线帽: 短接 PWR 侧 (启用板上 LED)
 *  ⚠ 供电: 可用 Arduino 5V (电流 ~200mA), 大扭矩建议外接 5V 电源
 *
 * ============== 串口指令 ==============
 *
 *  f   — 正转 1 圈 (forward)
 *  b   — 反转 1 圈 (backward)
 *  180 — 转 180 步
 *  s   — 停止
 *
 * ============== 电机参数 ==============
 *
 *  步距角: 5.625° / 64 (带减速比 1/64)
 *  减速后一圈: 4096 步 (half-step) / 2048 步 (full-step)
 *  供电: 5V DC
 *  相数: 4 相 5 线
 */

#define IN1 2
#define IN2 3
#define IN3 4
#define IN4 5

// 一圈 4096 个半步
#define STEPS_PER_REV 4096

// 8 拍 (half-step) 序列: 更平滑
const uint8_t sequence[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1},
};

int currentStep = 0;
int remainingSteps = 0;
int direction = 1;  // 1=正转, -1=反转

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  allOff();
  Serial.begin(9600);
  Serial.println(F("28BYJ-48 Stepper Ready"));
  Serial.println(F("  f=正转1圈  b=反转1圈  N=转N步  s=停"));
}

void loop() {
  // 处理串口命令
  if (Serial.available()) {
    handleCommand();
  }

  // 执行步进
  if (remainingSteps > 0) {
    currentStep = (currentStep + direction + 8) % 8;
    setStep(currentStep);
    remainingSteps--;
    delayMicroseconds(1200);  // 速度 (~1.2ms/步)
  }
  if (remainingSteps == 0) {
    allOff();  // 省电
  }
}

void setStep(int s) {
  digitalWrite(IN1, sequence[s][0]);
  digitalWrite(IN2, sequence[s][1]);
  digitalWrite(IN3, sequence[s][2]);
  digitalWrite(IN4, sequence[s][3]);
}

void allOff() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void handleCommand() {
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.length() == 0) return;

  if (cmd == "f" || cmd == "F") {
    direction = 1;
    remainingSteps = STEPS_PER_REV;
    Serial.println(F("Forward 1 rev"));
  }
  else if (cmd == "b" || cmd == "B") {
    direction = -1;
    remainingSteps = STEPS_PER_REV;
    Serial.println(F("Backward 1 rev"));
  }
  else if (cmd == "s" || cmd == "S") {
    remainingSteps = 0;
    allOff();
    Serial.println(F("Stop"));
  }
  else {
    int steps = cmd.toInt();
    if (steps != 0) {
      direction = (steps > 0) ? 1 : -1;
      remainingSteps = abs(steps);
      Serial.print(direction > 0 ? F("Forward ") : F("Backward "));
      Serial.print(remainingSteps);
      Serial.println(F(" steps"));
    }
  }
}
