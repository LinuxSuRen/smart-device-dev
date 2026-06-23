#include <LiquidCrystal.h>

const int TRIG_PIN = 9;
const int ECHO_PIN = 8;

const int RS = 7;
const int E  = 6;
const int D4 = 5;
const int D5 = 4;
const int D6 = 3;
const int D7 = 2;

LiquidCrystal lcd(RS, E, D4, D5, D6, D7);

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Distance:");
}

void loop() {
  long dist = getDistance();

  lcd.setCursor(0, 1);
  lcd.print("           ");

  lcd.setCursor(0, 1);
  if (dist < 0) {
    lcd.print("Out of range");
  } else {
    lcd.print(dist);
    lcd.print(" cm");
  }

  delay(500);
}

long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long dur = pulseIn(ECHO_PIN, HIGH, 30000);
  if (dur == 0) return -1;
  return dur / 58;
}
