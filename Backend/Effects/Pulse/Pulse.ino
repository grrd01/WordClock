/////////////////////////////////////////////
//
// LOLIN (WEMOS) D1 mini Lite (ESP8266) Pulse-Effect
//
/////////////////////////////////////////////

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>  // v1.10.4


char version[] = "V1";

// function definitions
static uint32_t Black = Adafruit_NeoPixel::Color(0, 0, 0);
static uint32_t White = Adafruit_NeoPixel::Color(49, 52, 34);
int satzalt[] = {0, 1, 3, 4, 5, 6, 22, 23, 24, 25, 26, 27, 42, 41, 59, 58, 57, 56, -1};
int satzneu[] = {0, 1, 3, 4, 5, 6, 8, 9, 10, 30, 31, 32, 39, 38, 37, 36, 35, 68, 69, 70, 71, 72, 73, -1};

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(121, D7, NEO_GRB + NEO_KHZ800);
uint32_t foregroundColor = White;
uint32_t backgroundColor = Black;
const uint16_t NUM_PIXELS = 121;

void lightup(int *word, uint32_t color) {
  for (int x = 0; x < pixels.numPixels() + 1; x++) {
    if (word[x] == -1) {
      break;
    } else {
      pixels.setPixelColor(word[x], color);
    }
  }
}


/**
 * Sets all pixels to the background and displays it
 */
void wipe() {
  for (int x = 0; x < pixels.numPixels(); ++x) {
    pixels.setPixelColor(x, backgroundColor);
  }
  pixels.show();
}

void setupDisplay() {
  pixels.begin();
  wipe();
}

// Hilfsfunktion: Prüft, ob LED in Array enthalten ist
bool inArray(int led, int *arr) {
  for (int i = 0; arr[i] != -1; i++) {
    if (arr[i] == led) return true;
  }
  return false;
}

// Hilfsfunktion: Matrix-Koordinaten (x, y) auf LED-Index abbilden
uint16_t xyToIndex(uint8_t x, uint8_t y) {
  if (y % 2 == 0) {
    // Gerade Zeile: links nach rechts
    return y * 11 + x;
  } else {
    // Ungerade Zeile: rechts nach links
    return y * 11 + (10 - x);
  }
}

// Hilfsfunktion: Helligkeit skalieren
uint32_t scaleColor(uint32_t color, float brightness) {
  uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * brightness);
  uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * brightness);
  uint8_t b = (uint8_t)((color & 0xFF) * brightness);
  return pixels.Color(r, g, b);
}

void wave() {
  float radius = 0.0f;
  const float speed = 0.10f; // Geschwindigkeit der Welle
  const float maxRadius = 9.0f; // maximaler Abstand von Mitte zu Ecke
  const float width = 1.3f; // Breite der Welle
  const uint8_t cx = 5, cy = 5; // Mittelpunkt

  // Statusarrays: markiere satzalt und satzneu Pixel und ob ihr Peak erreicht wurde
  bool inSatzneu[NUM_PIXELS];
  bool peakReached[NUM_PIXELS];
  for (uint16_t i = 0; i < NUM_PIXELS; ++i) {
    inSatzneu[i] = false;
    peakReached[i] = false;
  }

  const float peakThreshold = 0.95f; // Schwelle um "Maximalhelligkeit" zu erkennen
  const float peakEpsilon = 0.05f; // wie nah an diff==0 gelten wir als Peak

  while (radius < maxRadius + width * 2.0f) {
    for (uint8_t y = 0; y < 11; y++) {
      for (uint8_t x = 0; x < 11; x++) {
        float dx = x - cx;
        float dy = y - cy;
        float dist = sqrtf(dx * dx + dy * dy);
        float diff = dist - radius;
        float brightness = expf(-0.5f * (diff * diff) / (width * width));
        if (brightness < 0.01f) brightness = 0.0f;
        uint16_t idx = xyToIndex(x, y);

        // Priorität: satzalt (darf bis Peak hell bleiben und danach abdunkeln)
        if (!peakReached[idx] && (fabsf(diff) <= peakEpsilon || brightness >= peakThreshold)) peakReached[idx] = true;

        if (inArray(idx, satzalt) && !peakReached[idx]) {
          // nichts tun, bleibt hell bis Peak
        } else if (inArray(idx, satzneu) && peakReached[idx]) {
          // satzneu nach Peak: dauerhaft weiß
          pixels.setPixelColor(idx, foregroundColor);
        } else {
          // normale Pixel folgen der Welle
          pixels.setPixelColor(idx, scaleColor(foregroundColor, brightness));
        }

      }
    }
    pixels.show();
    radius += speed;
    delay(10);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Version: ");
  Serial.println(version);
  setupDisplay();
  lightup(satzalt, foregroundColor);
  pixels.show();
  delay(2000);
  wave();
}
void loop() {
}