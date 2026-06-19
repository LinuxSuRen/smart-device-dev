/*
 * FC-22 气体传感器 + 蜂鸣器报警 (+ 串口JSON输出)
 *
 * ============== 硬件接线 ==============
 *
 *  FC-22      ->  Arduino Uno
 *  -----------------------------
 *  AO         -> A1
 *  VCC        -> 5V
 *  GND        -> GND
 *  DO         -> 悬空
 *
 *  蜂鸣器     ->  Arduino Uno
 *  -----------------------------
 *  + (正极)   -> D8
 *  - (负极)   -> GND
 *
 * ============== 串口输出 ==============
 *  9600bps, 每5秒输出一行 JSON
 *  格式: {"ao":512,"delta":0,"level":0,"label":"SAFE","baseline":512}
 */

#define BUZZER_PIN       8
#define GAS_AO_PIN       A1
#define PREHEAT_SECONDS  30

int baseline = 0;
int currentAO = 0;
int currentDelta = 0;
int currentLevel = 0;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.begin(9600);
  Serial.println(F("FC-22 Gas Sensor"));

  // 自检
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH); delay(200);
    digitalWrite(BUZZER_PIN, LOW);  delay(150);
  }

  // 预热 + 基线
  Serial.print(F("Preheating"));
  long sum = 0;
  for (int i = 0; i < PREHEAT_SECONDS; i++) {
    sum += analogRead(GAS_AO_PIN);
    Serial.print('.');
    delay(1000);
  }
  baseline = sum / PREHEAT_SECONDS;
  Serial.println();
  Serial.print(F("Baseline: "));
  Serial.println(baseline);

  digitalWrite(BUZZER_PIN, HIGH); delay(300);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println(F("Ready"));
}

void loop() {
  readSensor();
  updateBuzzer();
  delay(50);
}

void readSensor() {
  currentAO = analogRead(GAS_AO_PIN);
  currentDelta = currentAO - baseline;
  if (currentDelta < 0) currentDelta = 0;

  if      (currentDelta >= 250) currentLevel = 3;
  else if (currentDelta >= 150) currentLevel = 2;
  else if (currentDelta >= 80)  currentLevel = 1;
  else                          currentLevel = 0;

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 5000) {
    Serial.print(F("{\"ao\":"));
    Serial.print(currentAO);
    Serial.print(F(",\"delta\":"));
    Serial.print(currentDelta);
    Serial.print(F(",\"level\":"));
    Serial.print(currentLevel);
    Serial.print(F(",\"label\":\""));
    switch (currentLevel) {
      case 3: Serial.print(F("HIGH"));   break;
      case 2: Serial.print(F("MID"));    break;
      case 1: Serial.print(F("LOW"));    break;
      default: Serial.print(F("SAFE"));  break;
    }
    Serial.print(F("\",\"baseline\":"));
    Serial.print(baseline);
    Serial.println(F("}"));
    lastPrint = millis();
  }
}

void updateBuzzer() {
  unsigned int interval, duration;
  switch (currentLevel) {
    case 3: interval = 0;   duration = 0;   break;
    case 2: interval = 200; duration = 80;  break;
    case 1: interval = 600; duration = 100; break;
    case 0: interval = 0;   duration = 0;   break;
  }

  if (currentLevel == 3) {
    digitalWrite(BUZZER_PIN, HIGH);
    return;
  }
  if (currentLevel == 0) {
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }

  static unsigned long lastToggle = 0;
  static bool buzzerOn = false;
  unsigned long now = millis();
  if (now - lastToggle >= (buzzerOn ? duration : interval - duration)) {
    buzzerOn = !buzzerOn;
    digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);
    lastToggle = now;
  }
}
