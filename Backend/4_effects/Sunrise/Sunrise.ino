/////////////////////////////////////////////
//
// LOLIN (WEMOS) D1 mini Lite (ESP8266) Sunrise-Effect
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
uint32_t foregroundColor = Green;
uint32_t backgroundColor = Black;

// satzalt und satzneu Definitionen
int satzalt[] = {0, 1, 3, 4, 5, 6, 22, 23, 24, 25, 26, 27, 42, 41, 59, 58, 57, 56, -1};
int satzneu[] = {0, 1, 3, 4, 5, 6, 8, 9, 10, 30, 31, 32, 39, 38, 37, 36, 35, 68, 69, 70, 71, 72, 73, -1};

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

void lightup(int *word, uint32_t color) {
  for (int x = 0; x < pixels.numPixels() + 1; x++) {
    if (word[x] == -1) {
      break;
    } else {
      pixels.setPixelColor(word[x], color);
    }
  }
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

// Hilfsfunktion: Prüft, ob LED in satzneu enthalten ist
bool isInSatzNeu(uint16_t pixelIdx) {
  for (int x = 0; x < pixels.numPixels() + 1; x++) {
    if (satzneu[x] == -1) break;
    if (satzneu[x] == pixelIdx) return true;
  }
  return false;
}

float clamp01(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 1.0f) return 1.0f;
  return value;
}

float smoothStep01(float value) {
  value = clamp01(value);
  return value * value * (3.0f - 2.0f * value);
}

float mixFloat(float a, float b, float t) {
  return a + (b - a) * clamp01(t);
}

uint32_t blendColor(uint32_t from, uint32_t to, float t) {
  t = clamp01(t);
  uint8_t fromR = (from >> 16) & 0xFF;
  uint8_t fromG = (from >> 8) & 0xFF;
  uint8_t fromB = from & 0xFF;
  uint8_t toR = (to >> 16) & 0xFF;
  uint8_t toG = (to >> 8) & 0xFF;
  uint8_t toB = to & 0xFF;

  return pixels.Color(
    (uint8_t)(fromR + (toR - fromR) * t),
    (uint8_t)(fromG + (toG - fromG) * t),
    (uint8_t)(fromB + (toB - fromB) * t)
  );
}

void renderSunriseFrame(float skyProgress, float sunProgress) {
  const uint32_t nightTop = pixels.Color(0, 0, 2);
  const uint32_t nightBottom = pixels.Color(8, 0, 1);
  const uint32_t dawnTop = pixels.Color(24, 0, 8);
  const uint32_t dawnBottom = pixels.Color(110, 10, 0);
  const uint32_t dayTop = pixels.Color(0, 32, 115);
  const uint32_t dayBottom = pixels.Color(28, 95, 145);
  const uint32_t sunOrange = pixels.Color(220, 45, 0);
  const uint32_t sunYellow = pixels.Color(255, 205, 0);

  float easedSky = smoothStep01(skyProgress);
  float easedSun = smoothStep01(sunProgress);
  float sunCenterX = 5.0f;
  float sunCenterY = mixFloat(12.8f, 5.0f, easedSun);
  float sunRadius = mixFloat(2.2f, 4.5f, easedSun);
  float sunEdge = mixFloat(1.8f, 0.7f, easedSun);
  float glowStrength = mixFloat(0.25f, 0.06f, easedSun);
  float blueShift = smoothStep01((easedSun - 0.08f) / 0.92f);
  uint32_t sunColor = blendColor(sunOrange, sunYellow, easedSun);

  for (uint8_t y = 0; y < 11; y++) {
    float rowBlend = y / 10.0f;
    uint32_t nightRow = blendColor(nightTop, nightBottom, rowBlend);
    uint32_t dawnRow = blendColor(dawnTop, dawnBottom, rowBlend);
    uint32_t dayRow = blendColor(dayTop, dayBottom, rowBlend);

    float warmFront = easedSky * 13.0f - (10.0f - y);
    float dawnAmount = smoothStep01((warmFront + 1.8f) / 2.8f);
    uint32_t skyColor = blendColor(nightRow, dawnRow, dawnAmount);
    skyColor = blendColor(skyColor, dayRow, blueShift);

    for (uint8_t x = 0; x < 11; x++) {
      float dx = x - sunCenterX;
      float dy = y - sunCenterY;
      float distance = sqrtf(dx * dx + dy * dy);

      float glowMask = clamp01((sunRadius + 2.4f - distance) / 3.2f);
      glowMask = smoothStep01(glowMask) * glowStrength;
      uint32_t glowColor = blendColor(skyColor, sunColor, glowMask);

      float sunMask = clamp01((sunRadius + sunEdge - distance) / (sunEdge * 2.0f));
      sunMask = smoothStep01(sunMask);

      uint32_t finalColor = blendColor(glowColor, sunColor, sunMask);
      pixels.setPixelColor(xyToIndex(x, y), finalColor);
    }
  }

  pixels.show();
}

// Sunrise-Effect
void sunriseEffect() {
  const uint16_t skyFrames = 100;
  const uint16_t sunFrames = 190;

  for (uint16_t frame = 0; frame <= skyFrames; frame++) {
    float skyProgress = frame / (float)skyFrames;
    renderSunriseFrame(skyProgress, 0.0f);
    delay(22);
    yield();
  }

  for (uint16_t frame = 0; frame <= sunFrames; frame++) {
    float progress = frame / (float)sunFrames;
    renderSunriseFrame(1.0f, progress);
    delay(24);
    yield();
  }

  delay(2000);
}

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Version: ");
  Serial.println(version);
  pixels.begin();
  wipe();

  lightup(satzneu, foregroundColor);
  pixels.show();
  delay(4000);

  sunriseEffect();
}

void loop() {

}
