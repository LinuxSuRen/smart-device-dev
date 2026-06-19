/*
 * SIM900A GSM/GPRS 模块 — 初始化 + 网络信息打印 + 短信/电话示例
 *
 * ============== 硬件接线 ==============
 *
 *  SIM900A    ->  Arduino Uno
 *  ----------------------------
 *  VCC        ->  外部 5V 2A 电源（勿用 Arduino 5V）
 *  GND        ->  外部电源 GND + Arduino GND（共地）
 *  TXD        ->  D2 (SoftwareSerial RX)
 *  RXD        ->  D3 (SoftwareSerial TX)
 *
 * ============== 电源要求 ==============
 *  SIM900A 峰值电流可达 2A，必须使用独立 5V 电源供电。
 *  共地：模块 GND 必须与 Arduino GND 相连。
 *
 * ============== 串口输出 ==============
 *  自动检测 SIM900A 波特率 (9600/115200/19200/38400/57600/4800/2400)
 *  Arduino 串口监视器请设为 9600 bps
 *
 * ============== 拨打电话 ==============
 *  通过串口发送命令拨打电话：
 *    CALL:13800138000       拨号
 *    HANG                    挂断
 *
 * ============== 发送短信 ==============
 *  通过串口发送命令发送短信：
 *    SMS:13800138000:你好    发送中文短信
 *    SMS:13800138000:Hello   发送英文短信
 */

#include <SoftwareSerial.h>

#define SIM900_RX  2
#define SIM900_TX  3

SoftwareSerial sim900(SIM900_RX, SIM900_TX);

String sim900Response = "";
String phoneNumber = "";
String smsText = "";
unsigned long lastPing = 0;

const long BAUD_RATES[] = {9600, 115200, 19200, 38400, 57600, 4800, 2400};
const int  BAUD_COUNT   = sizeof(BAUD_RATES) / sizeof(BAUD_RATES[0]);
long sim900Baud = 9600;

void setup() {
  Serial.begin(9600);

  // 等待串口就绪（部分开发板需要），最多等3秒
  for (int i = 0; i < 30; i++) {
    if (Serial) break;
    delay(100);
  }

  Serial.println(F("\n=============================="));
  Serial.println(F("  SIM900A GSM/GPRS Module"));
  Serial.println(F("=============================="));

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  long detected = sim900AutoBaud();
  if (detected > 0) {
    sim900Baud = detected;
    Serial.print(F("Module @ "));
    Serial.print(sim900Baud);
    Serial.println(F(" bps"));
    Serial.println();
  } else {
    Serial.println(F("[FAIL] Module not responding."));
    Serial.println(F("Check wiring, power supply (5V/2A), and SIM card."));
    while (1) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(200);
    }
  }

  digitalWrite(LED_BUILTIN, LOW);
  Serial.println(F("----- Module Info -----"));
  Serial.println();

  sim900GetIMEI();
  delay(500);

  sim900GetSignalQuality();
  delay(500);

  sim900GetNetworkRegistration();
  delay(500);

  sim900GetOperator();
  delay(500);

  sim900GetBatteryStatus();
  delay(500);

  Serial.println(F("----- Ready ------"));
  Serial.println(F("Commands:"));
  Serial.println(F("  CALL:13800138000   Make a call"));
  Serial.println(F("  HANG                Hang up"));
  Serial.println(F("  SMS:13800138000:Hi  Send SMS"));
  Serial.println();
}

void loop() {
  if (sim900.available()) {
    Serial.write(sim900.read());
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.length() == 0) return;

    if (cmd.startsWith(F("CALL:"))) {
      phoneNumber = cmd.substring(5);
      sim900MakeCall(phoneNumber);
    } else if (cmd == F("HANG")) {
      sim900HangUp();
    } else if (cmd.startsWith(F("SMS:"))) {
      int sep = cmd.indexOf(':', 4);
      if (sep > 0) {
        phoneNumber = cmd.substring(4, sep);
        smsText = cmd.substring(sep + 1);
        sim900SendSMS(phoneNumber, smsText);
      } else {
        Serial.println(F("Usage: SMS:13800138000:Message"));
      }
    } else {
      sim900.println(cmd);
    }
  }

  if (millis() - lastPing > 30000) {
    sim900ATTest();
    lastPing = millis();
  }
}

// ===================== 自动波特率检测 =====================
long sim900AutoBaud() {
  for (int b = 0; b < BAUD_COUNT; b++) {
    sim900.begin(BAUD_RATES[b]);
    delay(200);

    // 清空缓冲区
    while (sim900.available()) sim900.read();

    // 尝试不同次数，有的模块需要同步波特率
    for (int attempt = 0; attempt < 3; attempt++) {
      sim900.println(F("AT"));
      if (sim900WaitResponse(500, F("OK"))) {
        sim900Flush();
        return BAUD_RATES[b];
      }
      delay(100);
    }

    sim900.end();
    delay(100);
  }
  return -1;
}

// ===================== AT 测试 =====================
bool sim900ATTest() {
  sim900.println(F("AT"));
  return sim900WaitResponse(1000, F("OK"));
}

// ===================== 获取 IMEI =====================
void sim900GetIMEI() {
  sim900.println(F("AT+GSN"));
  if (sim900WaitResponse(3000, F("OK"))) {
    Serial.print(F("  IMEI: "));
    Serial.println(sim900Response);
  } else {
    Serial.println(F("  IMEI: read failed"));
  }
  sim900Flush();
  Serial.println();
}

// ===================== 获取信号强度 =====================
void sim900GetSignalQuality() {
  sim900.println(F("AT+CSQ"));
  if (sim900WaitResponse(3000, F("OK"))) {
    int idx = sim900Response.indexOf(F("+CSQ:"));
    if (idx >= 0) {
      int comma = sim900Response.indexOf(',', idx);
      String rssiStr = sim900Response.substring(idx + 5, comma);
      int rssi = rssiStr.toInt();
      // 0: -113 dBm or less, 1: -111 dBm, 2-30: -109 to -53 dBm,
      // 31: -51 dBm or greater, 99: not known
      int dBm = -113 + (rssi * 2);
      Serial.print(F("  Signal: "));
      Serial.print(rssi);
      Serial.print(F(" ("));

      if (rssi == 99) {
        Serial.print(F("no signal"));
      } else if (rssi >= 20) {
        Serial.print(F("excellent"));
      } else if (rssi >= 14) {
        Serial.print(F("good"));
      } else if (rssi >= 10) {
        Serial.print(F("fair"));
      } else if (rssi >= 1) {
        Serial.print(F("weak"));
      } else {
        Serial.print(F("extreme"));
      }
      Serial.print(F(", ~"));
      Serial.print(dBm);
      Serial.println(F(" dBm)"));
    }
  } else {
    Serial.println(F("  Signal: read failed"));
  }
  sim900Flush();
  Serial.println();
}

// ===================== 获取网络注册状态 =====================
void sim900GetNetworkRegistration() {
  sim900.println(F("AT+CREG?"));
  if (sim900WaitResponse(2000, F("OK"))) {
    int idx = sim900Response.indexOf(F("+CREG:"));
    if (idx >= 0) {
      int comma1 = sim900Response.indexOf(',', idx);
      int comma2 = sim900Response.indexOf(',', comma1 + 1);
      String stat = sim900Response.substring(comma1 + 1, comma2 > 0 ? comma2 : comma1 + 3);
      int state = stat.toInt();
      Serial.print(F("  Network: "));
      switch (state) {
        case 0: Serial.println(F("not registered, not searching")); break;
        case 1: Serial.println(F("registered (home network)"));     break;
        case 2: Serial.println(F("not registered, searching..."));  break;
        case 3: Serial.println(F("registration denied"));           break;
        case 5: Serial.println(F("registered (roaming)"));          break;
        default: Serial.println(F("unknown"));                       break;
      }
    }
  } else {
    Serial.println(F("  Network: read failed"));
  }
  sim900Flush();
  Serial.println();
}

// ===================== 获取运营商 =====================
void sim900GetOperator() {
  sim900.println(F("AT+COPS?"));
  if (sim900WaitResponse(5000, F("OK"))) {
    int idx = sim900Response.indexOf(F("+COPS:"));
    if (idx >= 0) {
      int q1 = sim900Response.indexOf('"', idx);
      int q2 = sim900Response.indexOf('"', q1 + 1);
      Serial.print(F("  Operator: "));
      if (q1 > 0 && q2 > q1) {
        Serial.println(sim900Response.substring(q1 + 1, q2));
      } else {
        Serial.println(F("unknown"));
      }
    }
  } else {
    Serial.println(F("  Operator: read failed"));
  }
  sim900Flush();
  Serial.println();
}

// ===================== 电池 / 电压状态 =====================
void sim900GetBatteryStatus() {
  sim900.println(F("AT+CBC"));
  if (sim900WaitResponse(2000, F("OK"))) {
    int idx = sim900Response.indexOf(F("+CBC:"));
    if (idx >= 0) {
      int comma1 = sim900Response.indexOf(',', idx);
      int comma2 = sim900Response.indexOf(',', comma1 + 1);
      String voltStr = sim900Response.substring(comma1 + 1, comma2);
      float volt = voltStr.toInt() / 1000.0;
      Serial.print(F("  Battery: "));
      Serial.print(volt, 2);
      Serial.println(F(" V"));
    }
  } else {
    Serial.println(F("  Battery: read failed"));
  }
  sim900Flush();
  Serial.println();
}

// ===================== 拨号 =====================
void sim900MakeCall(String number) {
  Serial.print(F("Calling "));
  Serial.print(number);
  Serial.println(F("..."));
  sim900.print(F("ATD"));
  sim900.print(number);
  sim900.println(F(";"));
}

// ===================== 挂断 =====================
void sim900HangUp() {
  Serial.println(F("Hanging up..."));
  sim900.println(F("ATH"));
}

// ===================== 发送短信 =====================
void sim900SendSMS(String number, String text) {
  Serial.print(F("Sending SMS to "));
  Serial.print(number);
  Serial.print(F(": "));
  Serial.println(text);

  sim900.println(F("AT+CMGF=1"));
  delay(100);
  sim900Flush();

  sim900.print(F("AT+CMGS=\""));
  sim900.print(number);
  sim900.println(F("\""));
  delay(100);

  sim900.print(text);
  delay(100);

  sim900.write(26);  // Ctrl+Z
  delay(3000);
  sim900Flush();
  Serial.println(F("  SMS sent"));
}

// ===================== 响应缓冲区 =====================
bool sim900WaitResponse(unsigned long timeout, const __FlashStringHelper *keyword) {
  sim900Response = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (sim900.available()) {
      char c = sim900.read();
      sim900Response += c;
    }
    if (sim900Response.indexOf(keyword) >= 0) return true;
  }
  return false;
}

void sim900Flush() {
  while (sim900.available()) sim900.read();
}
