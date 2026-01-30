#include <Arduino.h>
#include <Adafruit_NeoPixel.h>  // v1.10.4

// function definitions
static uint32_t Black = Adafruit_NeoPixel::Color(0, 0, 0);
static uint32_t White = Adafruit_NeoPixel::Color(49, 52, 34);
static uint32_t Green = Adafruit_NeoPixel::Color(10, 90, 0);
static uint32_t Red = Adafruit_NeoPixel::Color(90, 0, 0);
static uint32_t Blue = Adafruit_NeoPixel::Color(0, 20, 85);

int satzalt[] = {0, 1, 3, 4, 5, 6, 22, 23, 24, 25, 26, 27, 42, 41, 59, 58, 57, 56, -1};
int satzneu[] = {0, 1, 3, 4, 5, 6, 8, 9, 10, 30, 31, 32, 39, 38, 37, 36, 35, 68, 69, 70, 71, 72, 73, -1};

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(121, D7, NEO_GRB + NEO_KHZ800);
uint32_t foregroundColor = White;
uint32_t backgroundColor = Black;

void lightup(int *word, uint32_t color) {
  for (int x = 0; x < pixels.numPixels() + 1; x++) {
    if (word[x] == -1) {
      break;
    } else {
      pixels.setPixelColor(word[x], color);
    }
  }
}

// Sets all pixels to the background
void blank() {
  for (int x = 0; x < pixels.numPixels(); ++x) {
    pixels.setPixelColor(x, backgroundColor);
  }
}

// Sets all pixels to the background and displays it
void wipe() {
  blank();
  pixels.show();
}

void setup() {
  Serial.begin(115200);
  Serial.println("");
  pixels.begin();
  wipe();
  lightup(satzalt, foregroundColor);
  pixels.show();
}

void loop() {
  snowflakes();
  delay(20);
}

// Konfiguration
#define MAX_SNOWFLAKES 8  // Anzahl der gleichzeitigen Flocken

struct Snowflake {
  float x, y;       // Position
  float speed;      // Fallgeschwindigkeit
  float drift;      // Seitwärtsbewegung
  uint8_t bright;   // Helligkeit
  bool active;      // Status
};

Snowflake flakes[MAX_SNOWFLAKES];

// Hilfsfunktion für das Schlangenmuster (Serpentine)
int getIndex(int x, int y) {
  if (x < 0 || x > 10 || y < 0 || y > 10) return -1;
  return (y % 2 == 1) ? (y * 11) + (10 - x) : (y * 11) + x;
}

void snowflakes() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 50) return;
  lastUpdate = millis();

  pixels.clear();

  for (int i = 0; i < MAX_SNOWFLAKES; i++) {
    if (!flakes[i].active) {
      flakes[i].x = random(0, 110) / 10.0;
      flakes[i].y = -1.0;
      flakes[i].speed = (random(4, 12) / 100.0);
      flakes[i].drift = (random(-4, 5) / 100.0);

      // DEUTLICHERE UNTERSCHIEDE:
      // Ein paar Flocken sind sehr dunkel (30), andere sehr hell (255)
      flakes[i].bright = random(30, 256);

      flakes[i].active = true;
    }

    flakes[i].y += flakes[i].speed;
    flakes[i].x += flakes[i].drift;

    int ix = (int)(flakes[i].x + 0.5); // Runden für bessere Platzierung
    int iy = (int)(flakes[i].y + 0.5);

    if (iy >= 0 && iy <= 10 && ix >= 0 && ix <= 10) {
      int idx = getIndex(ix, iy);
      if (idx != -1) {
        // Hier wird die individuelle Helligkeit angewendet
        uint8_t b = flakes[i].bright;
        pixels.setPixelColor(idx, pixels.Color(b, b, b));
      }
    }

    if (flakes[i].y > 11.0 || flakes[i].x < -0.5 || flakes[i].x > 10.5) {
      flakes[i].active = false;
    }
  }

  pixels.show();
}