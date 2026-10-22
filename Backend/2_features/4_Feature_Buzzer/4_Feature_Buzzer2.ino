/*
 * ==================================================================================
 * Passive Buzzer an Wemos D1 mini
 * Verbindungsschema:
 * - VCC/5V          -> VCC
 * - D6/GPIO12/MISO  -> SDA
 * - GND/G           -> GND
 * ==================================================================================
 */

#define BUZZER_PIN D6

const int starWarsMelody[] = {
  440, 440, 440,
  349, 523, 440,
  349, 523, 440
};

const int starWarsDurations[] = {
  500, 500, 500,
  350, 150, 500,
  350, 150, 650
};

int starWarsIndex = 0;
unsigned long starWarsLastSwitch = 0;
bool starWarsPlaying = false;

void startStarWarsNote() {
  tone(BUZZER_PIN, starWarsMelody[starWarsIndex]);
  starWarsLastSwitch = millis();
  starWarsPlaying = true;
}

void starwars() {
  if (!starWarsPlaying) {
    startStarWarsNote();
    return;
  }

  if (millis() - starWarsLastSwitch >= starWarsDurations[starWarsIndex]) {
    noTone(BUZZER_PIN);

    starWarsIndex++;

    if (starWarsIndex >= (sizeof(starWarsMelody) / sizeof(starWarsMelody[0]))) {
      starWarsIndex = 0;
      starWarsPlaying = false;
      return;
    }

    startStarWarsNote();
  }
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  starwars();
  // do stuff here...
}
