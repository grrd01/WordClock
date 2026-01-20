/////////////////////////////////////////////
//
// LOLIN (WEMOS) D1 mini Lite (ESP8266) Rainbow-Effect
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

// Hilfsfunktion: HSV zu RGB (vereinfachte Version für NeoPixel)
uint32_t colorWheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return pixels.Color(255 - pos * 3, 0, pos * 3);
  }
  if (pos < 170) {
    pos -= 85;
    return pixels.Color(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return pixels.Color(pos * 3, 255 - pos * 3, 0);
}

void loop() {
  static uint16_t frame = 0;
  for (uint8_t y = 0; y < 11; y++) {
    for (uint8_t x = 0; x < 11; x++) {
      // Diagonale bestimmen: x + y
      uint8_t diag = x + y;
      // Regenbogenfarbe berechnen, animiert durch frame
      uint8_t colorPos = (diag * 20 + frame) % 256;
      uint32_t color = colorWheel(colorPos);
      uint16_t idx = xyToIndex(x, y);
      pixels.setPixelColor(idx, color);
    }
  }
  pixels.show();
  delay(30);
  frame++;
}
