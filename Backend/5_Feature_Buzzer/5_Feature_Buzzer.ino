/*
 * ==================================================================================
 * Passive Buzzer an Wemos D1 mini
 * Verbindungsschema:
 * - VCC/5V     -> VCC
 * - D6/GPI12/MISO  -> SDA
 * - GND/G      -> GND
 * ==================================================================================
 */

#define BUZZER_PIN D5

void beep() {
  tone(BUZZER_PIN, 1000); // 1 kHz
  delay(500);
  noTone(BUZZER_PIN);
  delay(500);
}

void alert() {
  // Rising frequency
  for (int freq = 500; freq <= 1500; freq += 50) {
    tone(BUZZER_PIN, freq);
    delay(20);
  }
  // Falling frequency
  for (int freq = 1500; freq >= 500; freq -= 50) {
    tone(BUZZER_PIN, freq);
    delay(20);
  }
}

void police() {
  // High tone
  tone(BUZZER_PIN, 800);
  delay(500);

  // Low tone
  tone(BUZZER_PIN, 500);
  delay(500);
}

void playTone(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
  delay(duration * 1.3);
}

void mario() {
  playTone(660, 100);
  playTone(660, 100);
  delay(100);
  playTone(660, 100);

  delay(150);
  playTone(510, 100);
  playTone(660, 100);
  delay(150);
  playTone(770, 100);

  delay(300);
  playTone(380, 100);

  delay(500); // Wait and loop
}

void starwars() {
  playTone(440, 500);  // A
  playTone(440, 500);  // A
  playTone(440, 500);  // A

  playTone(349, 350);  // F
  playTone(523, 150);  // C
  playTone(440, 500);  // A

  playTone(349, 350);  // F
  playTone(523, 150);  // C
  playTone(440, 650);  // A (long note)

  delay(1000);     // Pause before repeating
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  for (int i = 0; i < 10; i++) {
     beep();
  }
  delay(500);
  for (int i = 0; i < 10; i++) {
    alert();
  }
  delay(500);
  for (int i = 0; i < 10; i++) {
    police();
  }
  delay(500);
  for (int i = 0; i < 3; i++) {
    mario();
  }
  delay(500);
  for (int i = 0; i < 3; i++) {
    starwars();
  }
}

