/*
 * ==================================================================================
 * Adafruit_NeoPixel WS2812B an Wemos D1 mini
 * Verbindungsschema:
* - VCC/5V         -> 5V
* - D7/GPIO13/MOSI -> DIN
* - GND            -> GND
 * ==================================================================================
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>  // v1.10.4

static uint32_t Black = Adafruit_NeoPixel::Color(0, 0, 0);
static uint32_t White = Adafruit_NeoPixel::Color(49, 52, 34);
static uint32_t Green = Adafruit_NeoPixel::Color(10, 90, 0);
static uint32_t Red = Adafruit_NeoPixel::Color(90, 0, 0);
static uint32_t Blue = Adafruit_NeoPixel::Color(0, 20, 85);

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(121, D7, NEO_GRB + NEO_KHZ800);

/**
 * Runs through all pixels
 * @param color
 */
void chase(uint32_t color) {
  for (uint16_t i = 0; i < pixels.numPixels() + 4; i++) {
    pixels.setPixelColor(i, color); // Draw new pixel
    pixels.setPixelColor(i - 4, Black); // Erase pixel a few steps back
    pixels.show();
    delay(25);
  }
}

/**
 * Sets all pixels to black and displays it
 */
void wipe() {
   for (int x = 0; x < pixels.numPixels(); ++x) {
     pixels.setPixelColor(x, Black);
   }
  pixels.show();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Script gestartet");
  pixels.begin();
  wipe();
  chase(Green);
}

void loop() {

}
