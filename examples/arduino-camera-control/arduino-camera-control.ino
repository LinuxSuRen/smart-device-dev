const int SOUND_PIN = 2;
const int RELAY_PIN = 3;
const int TRIG_PIN = 4;
const int ECHO_PIN = 5;

const int DIST_THRESHOLD = 100;
const unsigned long TIMEOUT_MS = 5 * 60 * 1000UL;

unsigned long lastTriggerTime = 0;

bool prevSound = false;
bool prevPerson = false;
bool prevCamState = false;

void setup() {
  Serial.begin(9600);
  pinMode(SOUND_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(TRIG_PIN, LOW);

  Serial.println("System ready");
}

long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long dur = pulseIn(ECHO_PIN, HIGH, 6000);
  if (dur == 0) return -1;
  return dur / 58;
}

void loop() {
  bool sound = digitalRead(SOUND_PIN) == HIGH;

  long dist = getDistance();
  bool person = (dist > 0 && dist < DIST_THRESHOLD);

  if (sound && !prevSound) {
    Serial.println("> Sound detected");
    prevSound = true;
  }
  if (!sound && prevSound) {
    Serial.println("> Sound stopped");
    prevSound = false;
  }

  if (person && !prevPerson) {
    Serial.print("> Person detected (");
    Serial.print(dist);
    Serial.println("cm)");
    prevPerson = true;
  }
  if (!person && prevPerson) {
    Serial.println("> Person left");
    prevPerson = false;
  }

  bool trigger = sound && person;
  if (trigger) {
    lastTriggerTime = millis();
    if (!prevCamState) {
      digitalWrite(RELAY_PIN, LOW);
      prevCamState = true;
      Serial.println(">> Camera ON");
    }
  }

  if (prevCamState && millis() - lastTriggerTime > TIMEOUT_MS) {
    digitalWrite(RELAY_PIN, HIGH);
    prevCamState = false;
    Serial.println(">> Camera OFF (timeout)");
  }

  delay(200);
}
