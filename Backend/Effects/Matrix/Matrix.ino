/////////////////////////////////////////////
//
// LOLIN (WEMOS) D1 mini Lite (ESP8266) Matrix-Effect
//
/////////////////////////////////////////////

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>  // v1.10.4

char version[] = "V1";

// function definitions
static uint32_t Black = Adafruit_NeoPixel::Color(0, 0, 0);
static uint32_t White = Adafruit_NeoPixel::Color(49, 52, 34);
static uint32_t Green = Adafruit_NeoPixel::Color(10, 90, 0);
static uint32_t Red = Adafruit_NeoPixel::Color(90, 0, 0);
static uint32_t Blue = Adafruit_NeoPixel::Color(0, 20, 85);

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(121, D7, NEO_GRB + NEO_KHZ800);
uint32_t foregroundColor = White;
uint32_t backgroundColor = Black;

/**
 * Runs through all pixels
 * @param color
 */
void chase(uint32_t color) {
  for (uint16_t i = 0; i < pixels.numPixels() + 4; i++) {
    pixels.setPixelColor(i, color); // Draw new pixel
    pixels.setPixelColor(i - 4, backgroundColor); // Erase pixel a few steps back
    pixels.show();
    delay(25);
  }
}

/**
 * Sets all pixels to the background
 */
void blank() {
  for (int x = 0; x < pixels.numPixels(); ++x) {
    pixels.setPixelColor(x, backgroundColor);
  }
}

/**
 * Sets all pixels to the background and displays it
 */
void wipe() {
  blank();
  pixels.show();
}

void setupDisplay() {
  pixels.begin();
  wipe();
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

struct Drop {
  float y; // aktuelle Position (kann zwischen den Zeilen liegen)
  float speed; // Geschwindigkeit (Zeilen pro Frame)
  uint8_t length; // Abdunklungslänge
  uint8_t maxBrightness; // Maximale Helligkeit (0..255)
  uint16_t cooldown; // Frames bis zum nächsten Start
};

Drop drops[11];

void initDrop(uint8_t col) {
  drops[col].y = -random(1, 8); // Startet oberhalb der Matrix
  drops[col].speed = 0.05 + random(10, 40) / 100.0; // 0.15..0.45
  drops[col].length = random(3, 8); // 3..7
  drops[col].maxBrightness = random(120, 255); // 120..255
  drops[col].cooldown = random(10, 80); // Frames bis zum nächsten Drop
}

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Version: ");
  Serial.println(version);
  pixels.begin();
  wipe();
  for (uint8_t x = 0; x < 11; x++) {
    initDrop(x);
  }
}

void loop() {
  blank();
  for (uint8_t x = 0; x < 11; x++) {
    if (drops[x].cooldown > 0) {
      drops[x].cooldown--;
      continue;
    }
    drops[x].y += drops[x].speed;
    if (drops[x].y - drops[x].length > 10) {
      initDrop(x);
      continue;
    }
    for (int8_t i = 0; i < drops[x].length; i++) {
      int8_t py = (int8_t)(drops[x].y) - i;
      if (py < 0 || py > 10) continue;
      float fade = 1.0 - (float)i / drops[x].length;
      uint8_t brightness = (uint8_t)(drops[x].maxBrightness * fade);
      uint32_t col = pixels.Color(0, brightness, 0);
      uint16_t idx = xyToIndex(x, py);
      pixels.setPixelColor(idx, col);
    }
  }
  pixels.show();
  delay(30);
}
