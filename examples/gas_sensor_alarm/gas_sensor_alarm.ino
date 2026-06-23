/*
 * FC-22 气体传感器 + 蜂鸣器 浓度分级报警 + TMP1826 (V182) 温度传感器
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
 *   TMP1826 VDD  -> Arduino 5V (或 3.3V)
 *   TMP1826 GND  -> Arduino GND
 *   TMP1826 DQ   -> Arduino D2 (使用内部上拉, 无需外接电阻)
 *   5161BS COM(3,8) -> 100Ω 电阻 -> Arduino 5V    ← 共用限流电阻
 *   5161BS a~g     -> Arduino D3~D7, D9~D10       (段脚直连)
 *
 * 外部供电方式:
 *   1. DC 圆口 (7-12V)  -> 板载稳压到 5V
 *   2. Vin 引脚 (7-12V)  -> 同上
 *   3. 5V 引脚直接供电   -> 必须是稳压 5V, 跳过板载稳压器
 *   ❌ 不建议: 9V 方块电池 (电流太小, 带不动传感器)
 *   (DO 引脚已弃用，只靠 AO 模拟值判断，避免电位器误触发)
 *
 * 依赖库 (通过 Arduino 库管理器安装):
 *   - OneWire  (by Jim Studt)
 *   - DallasTemperature  (by Miles Burton)
 */
#include <OneWire.h>
#include <DallasTemperature.h>

#define BUZZER_PIN       8
#define GAS_AO_PIN      A1
#define PREHEAT_SECONDS  30
#define ONE_WIRE_BUS     2

#define SEG_A  3
#define SEG_B  4
#define SEG_C  5
#define SEG_D  6
#define SEG_E  7
#define SEG_F  9
#define SEG_G  10

const uint8_t segPins[7] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G};
// 共阳极: LOW=亮, HIGH=灭  顺序: a,b,c,d,e,f,g
const uint8_t digitPattern[10][7] = {
  {0,0,0,0,0,0,1}, // 0
  {1,0,0,1,1,1,1}, // 1
  {0,0,1,0,0,1,0}, // 2
  {0,0,0,0,1,1,0}, // 3
  {1,0,0,1,1,0,0}, // 4
  {0,1,0,0,1,0,0}, // 5
  {0,1,0,0,0,0,0}, // 6
  {0,0,0,1,1,1,1}, // 7
  {0,0,0,0,0,0,0}, // 8
  {0,0,0,0,1,0,0}, // 9
};

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int baseline = 0;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.begin(9600);
  Serial.println("FC-22 Gas Alarm + TMP1826 Temp Sensor");

  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
  sensors.begin();

  for (int i = 0; i < 7; i++) {
    pinMode(segPins[i], OUTPUT);
    digitalWrite(segPins[i], HIGH);
  }
  displayDigit(0);

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
      case 0: Serial.print("SAFE");   break;
      case 1: Serial.print("LOW !");  break;
      case 2: Serial.print("MID !!"); break;
      case 3: Serial.print("HIGH !!!"); break;
    }

    pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
    sensors.requestTemperatures();
    pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
    float tempC = sensors.getTempCByIndex(0);
    if (tempC != DEVICE_DISCONNECTED_C) {
      Serial.print("  Temp:");
      Serial.print(tempC);
      Serial.println(" C");
      int d = (int)tempC % 10;
      if (d < 0) d = 0;
      displayDigit(d);
    } else {
      Serial.println("  Temp: N/A");
      displayDigit(0);
    }
    lastPrint = millis();
  }
}

void displayDigit(int d) {
  if (d < 0 || d > 9) return;
  for (int i = 0; i < 7; i++) {
    digitalWrite(segPins[i], digitPattern[d][i]);
  }
}
