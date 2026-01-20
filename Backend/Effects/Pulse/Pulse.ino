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

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Version: ");
  Serial.println(version);
  setupDisplay();
  chase(Green); // run basic screen test and show success
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

void loop() {
  static float radius = 0.0;
  const float speed = 0.25; // Geschwindigkeit der Welle
  const float maxRadius = 7.8; // maximaler Abstand von Mitte zu Ecke
  const float width = 1.5; // Breite der Welle
  const uint8_t cx = 5, cy = 5; // Mittelpunkt
  for (uint8_t y = 0; y < 11; y++) {
    for (uint8_t x = 0; x < 11; x++) {
      float dx = x - cx;
      float dy = y - cy;
      float dist = sqrt(dx * dx + dy * dy);
      float diff = dist - radius;
      // Gauß-ähnliche Helligkeitsverteilung
      float brightness = exp(-0.5 * (diff * diff) / (width * width));
      if (brightness < 0.01) brightness = 0.0;
      uint32_t col = scaleColor(White, brightness);
      uint16_t idx = xyToIndex(x, y);
      pixels.setPixelColor(idx, col);
    }
  }
  pixels.show();
  radius += speed;
  if (radius > maxRadius + width * 2) radius = 0.0;
  delay(30);
}
