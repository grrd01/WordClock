const int TOUCH_PIN = D5; // Pin, an dem der Sensor hängt
const int LED_PIN = LED_BUILTIN; // Interne LED des Wemos

void setup() {
  pinMode(TOUCH_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  int touchState = digitalRead(TOUCH_PIN);

  if (touchState == HIGH) {
    digitalWrite(LED_PIN, LOW); // LED an (beim Wemos ist LOW = AN)
  } else {
    digitalWrite(LED_PIN, HIGH); // LED aus
  }
}