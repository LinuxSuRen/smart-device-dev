/*
 * FC-22 气体传感器 + 蜂鸣器 浓度分级报警 (独立运行版)
 *
 * 供电要求: 5V, 建议 >= 500mA (USB 充电头即可)
 *
 * 硬件接线:
 *   FC-22 VCC  -> Arduino 5V
 *   FC-22 GND  -> Arduino GND
 *   FC-22 AO   -> Arduino A1
 *   FC-22 DO   -> 不接
 *   蜂鸣器 +   -> Arduino D8
 *   蜂鸣器 -   -> GND
 *
 * 外部供电方式:
 *   1. DC 圆口 (7-12V)  -> 板载稳压到 5V
 *   2. Vin 引脚 (7-12V)  -> 同上
 *   3. 5V 引脚直接供电   -> 必须是稳压 5V, 跳过板载稳压器
 *   ❌ 不建议: 9V 方块电池 (电流太小, 带不动传感器)
 *   (DO 引脚已弃用，只靠 AO 模拟值判断，避免电位器误触发)
 */
#define BUZZER_PIN       8
#define GAS_AO_PIN      A1
#define PREHEAT_SECONDS  30

int baseline = 0;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.begin(9600);
  Serial.println("FC-22 Gas Alarm");

  // --- 自检: 3 声 ---
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH); delay(300);
    digitalWrite(BUZZER_PIN, LOW);  delay(200);
  }
  delay(500);

  // --- 预热 + 采集基线 ---
  Serial.print("Preheating");
  long sum = 0;
  int n = 0;
  for (int i = 0; i < PREHEAT_SECONDS; i++) {
    sum += analogRead(GAS_AO_PIN);
    n++;
    Serial.print(".");
    delay(1000);
  }
  baseline = sum / n;

  Serial.println();
  Serial.print("Baseline: ");
  Serial.print(baseline);
  Serial.print("  Threshold: +");
  Serial.println(80);
  Serial.println("Ready.");

  // 预热完成, 响一声
  digitalWrite(BUZZER_PIN, HIGH); delay(200);
  digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
  int ao = analogRead(GAS_AO_PIN);
  int delta = ao - baseline;
  if (delta < 0) delta = 0;

  int level;
  if      (delta >= 250) level = 3;  // 高浓度: 长鸣
  else if (delta >= 150) level = 2;  // 中浓度: 急促嘀
  else if (delta >= 80)  level = 1;  // 低浓度: 慢嘀
  else                   level = 0;  // 正常

  static unsigned long lastToggle = 0;
  static bool buzzerOn = false;

  unsigned int interval;  // 嘀+停 周期 (ms)
  unsigned int duration;  // 嘀的时长 (ms)

  switch (level) {
    case 3: interval = 0;   duration = 0;   break;
    case 2: interval = 200; duration = 80;  break;
    case 1: interval = 600; duration = 100; break;
    case 0: interval = 0;   duration = 0;   break;
  }

  if (level == 3) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else if (level == 0) {
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    unsigned long now = millis();
    if (now - lastToggle >= (buzzerOn ? duration : interval - duration)) {
      buzzerOn = !buzzerOn;
      digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);
      lastToggle = now;
    }
  }

  delay(50);

  // 每秒输出一次
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 1000) {
    Serial.print("AO:");
    Serial.print(ao);
    Serial.print(" (+");
    Serial.print(delta);
    Serial.print(") ");
    switch (level) {
      case 0: Serial.println("SAFE");   break;
      case 1: Serial.println("LOW !");  break;
      case 2: Serial.println("MID !!"); break;
      case 3: Serial.println("HIGH !!!"); break;
    }
    lastPrint = millis();
  }
}
