/*
 * ==================================================================================
 * non-blocking version: void loop is running continously to check for events
 * ==================================================================================
 * Passive Buzzer an Wemos D1 mini
 * Verbindungsschema:
 * - VCC/5V          -> VCC
 * - D6/GPIO12/MISO  -> SDA
 * - GND/G           -> GND
 * ==================================================================================
 */

#define BUZZER_PIN D6

int soundIndex = 0;

struct SoundStep {
  int frequency;
  unsigned long duration;
};

struct SoundDefinition {
  const SoundStep* steps;
  int length;
};

void applyStep(const SoundStep& step);

const SoundStep beepSequence[] = {
  {1000, 500},
  {0, 500}
};

const SoundStep sireneSequence[] = {
  {500, 20}, {550, 20}, {600, 20}, {650, 20}, {700, 20}, {750, 20}, {800, 20}, {850, 20}, {900, 20}, {950, 20},
  {1000, 20}, {1050, 20}, {1100, 20}, {1150, 20}, {1200, 20}, {1250, 20}, {1300, 20}, {1350, 20}, {1400, 20}, {1450, 20}, {1500, 20},
  {1450, 20}, {1400, 20}, {1350, 20}, {1300, 20}, {1250, 20}, {1200, 20}, {1150, 20}, {1100, 20}, {1050, 20}, {1000, 20},
  {950, 20}, {900, 20}, {850, 20}, {800, 20}, {750, 20}, {700, 20}, {650, 20}, {600, 20}, {550, 20}, {500, 20}
};

const SoundStep marioSequence[] = {
  {660, 130}, {0, 50}, {660, 130}, {0, 170}, {660, 130}, {0, 200},
  {510, 130}, {0, 50}, {660, 130}, {0, 200}, {770, 130}, {0, 440},
  {380, 130}, {0, 1400}
};

const SoundStep starWarsSequence[] = {
  {440, 500}, {0, 200}, {440, 500}, {0, 200}, {440, 500}, {0, 200},
  {349, 350}, {0, 150}, {523, 150}, {0, 50}, {440, 500}, {0, 200},
  {349, 350}, {0, 150}, {523, 150}, {0, 50}, {440, 650},
  {0, 2000}
};

const SoundDefinition sounds[] = {
  {beepSequence, sizeof(beepSequence) / sizeof(beepSequence[0])},
  {sireneSequence, sizeof(sireneSequence) / sizeof(sireneSequence[0])},
  {marioSequence, sizeof(marioSequence) / sizeof(marioSequence[0])},
  {starWarsSequence, sizeof(starWarsSequence) / sizeof(starWarsSequence[0])}
};

const int soundCount = sizeof(sounds) / sizeof(sounds[0]);

const SoundStep* activeSequence = nullptr;
int activeStep = 0;
int activeSequenceLength = 0;
unsigned long stepStartTime = 0;

void applyStep(const SoundStep& step) {
  if (step.frequency > 0) {
    tone(BUZZER_PIN, step.frequency);
  } else {
    noTone(BUZZER_PIN);
  }
}

void startSound(int newIndex) {
  if (newIndex < 0 || newIndex >= soundCount) {
    activeSequence = nullptr;
    activeSequenceLength = 0;
    noTone(BUZZER_PIN);
    return;
  }

  activeSequence = sounds[newIndex].steps;
  activeSequenceLength = sounds[newIndex].length;
  activeStep = 0;
  stepStartTime = millis();
  applyStep(activeSequence[activeStep]);
}

void updateSound() {
  if (activeSequence == nullptr) {
    return;
  }

  if (millis() - stepStartTime >= activeSequence[activeStep].duration) {
    activeStep++;

    if (activeStep >= activeSequenceLength) {
      activeStep = 0;
    }

    stepStartTime = millis();
    applyStep(activeSequence[activeStep]);
  }
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  startSound(soundIndex);
}

void loop() {
  updateSound();

  // später hier eine Funktion einbauen, welche soundIndex auf andere Werte setzen kann
  // z.B. if (sensorAktiv) soundIndex = 1;
  // if (soundIndex != oldIndex) startSound(soundIndex);
}
