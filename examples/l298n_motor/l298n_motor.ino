/*
 * L298N 双路直流电机驱动
 *
 * ============== 硬件接线 ==============
 *
 *  L298N 驱动板  ->   Arduino Uno      电源接线
 *  -------------------------------     -------------------
 *  ENA (使能A)  ->  D9  (PWM调速)      12V 或 +  -> 电池正极 (6-12V)
 *  IN1          ->  D8                  GND       -> 电池负极 + Arduino GND
 *  IN2          ->  D7                  5V 输出   -> Arduino 5V (跳线帽接上)
 *  IN3          ->  D6                 (⚠ L298N 的 GND 必须与 Arduino GND 共地)
 *  IN4          ->  D5
 *  ENB (使能B)  ->  D10 (PWM调速)
 *
 *  电机接线:
 *  电机 A (左)  ->  L298N OUT1 + OUT2
 *  电机 B (右)  ->  L298N OUT3 + OUT4
 *
 *  跳线帽:
 *  - ENA/ENB 上的跳线帽: 拔掉 (用 PWM 调速)
 *  - 5V 输出跳线帽: 接上 (L298N 给 Arduino 供 5V)
 *    或拔掉 (各自独立供电)
 *
 * ============== 串口指令 (9600bps) ==============
 *
 *  f     — 前进
 *  b     — 后退
 *  l     — 左转
 *  r     — 右转
 *  s     — 停止
 *  0-255 — 设置速度 (如 150 回车)
 */

#define ENA  9
#define IN1  8
#define IN2  7
#define IN3  6
#define IN4  5
#define ENB  10

int speed = 180;  // 默认速度 (0-255)

void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  stop();
  Serial.begin(9600);
  Serial.println(F("L298N Motor Driver Ready"));
  Serial.println(F("  f=前进 b=后退 l=左转 r=右转 s=停 N=速度"));
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() == 0) return;

    if (cmd == "f" || cmd == "F") {
      forward();
    } else if (cmd == "b" || cmd == "B") {
      backward();
    } else if (cmd == "l" || cmd == "L") {
      turnLeft();
    } else if (cmd == "r" || cmd == "R") {
      turnRight();
    } else if (cmd == "s" || cmd == "S") {
      stop();
    } else {
      int val = cmd.toInt();
      if (val >= 0 && val <= 255) {
        speed = val;
        Serial.print(F("Speed: "));
        Serial.println(speed);
      }
    }
  }
}

void forward() {
  Serial.print(F("Forward ("));
  Serial.print(speed);
  Serial.println(')');

  // 电机 A 正转
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  // 电机 B 正转
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void backward() {
  Serial.print(F("Backward ("));
  Serial.print(speed);
  Serial.println(')');

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void turnLeft() {
  Serial.print(F("Left ("));
  Serial.print(speed);
  Serial.println(')');

  // 左轮反转, 右轮正转 (原地左转)
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void turnRight() {
  Serial.print(F("Right ("));
  Serial.print(speed);
  Serial.println(')');

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void stop() {
  Serial.println(F("Stop"));

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}
