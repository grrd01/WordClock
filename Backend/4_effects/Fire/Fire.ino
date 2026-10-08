/////////////////////////////////////////////
//
// LOLIN (WEMOS) D1 mini Lite (ESP8266) Fire-Effect
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

const uint8_t MATRIX_WIDTH = 11;
const uint8_t MATRIX_HEIGHT = 11;

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

uint32_t scaleColor(uint32_t color, float brightness) {
  brightness = clamp01(brightness);
  uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * brightness);
  uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * brightness);
  uint8_t b = (uint8_t)((color & 0xFF) * brightness);
  return pixels.Color(r, g, b);
}

uint32_t fireColorFromHeat(uint8_t heat, uint8_t y, bool blueAccent) {
  const uint32_t ember = pixels.Color(18, 2, 0);
  const uint32_t orange = pixels.Color(140, 24, 0);
  const uint32_t warmOrange = pixels.Color(180, 58, 0);
  const uint32_t yellow = pixels.Color(220, 155, 8);
  const uint32_t hotYellow = pixels.Color(255, 220, 70);
  const uint32_t blueBase = pixels.Color(10, 65, 170);

  float heatLevel = heat / 255.0f;
  float heightFromBottom = (MATRIX_HEIGHT - 1 - y) / (float)(MATRIX_HEIGHT - 1);
  float upperCooling = clamp01(heightFromBottom * 0.85f);
  float shadedHeat = clamp01(heatLevel * (1.0f - upperCooling * 0.35f));
  uint32_t flameColor;

  if (shadedHeat < 0.25f) {
    flameColor = blendColor(Black, ember, shadedHeat / 0.25f);
  } else if (shadedHeat < 0.55f) {
    flameColor = blendColor(ember, orange, (shadedHeat - 0.25f) / 0.30f);
  } else if (shadedHeat < 0.82f) {
    flameColor = blendColor(orange, yellow, (shadedHeat - 0.55f) / 0.27f);
  } else {
    flameColor = blendColor(yellow, hotYellow, (shadedHeat - 0.82f) / 0.18f);
  }

  flameColor = blendColor(flameColor, warmOrange, upperCooling * 0.55f);

  if (blueAccent) {
    float accentStrength = 0.25f + heatLevel * 0.45f;
    flameColor = blendColor(flameColor, blueBase, accentStrength);
  }

  return scaleColor(flameColor, clamp01(heatLevel * 1.15f));
}

// Fire-Effect
void fireEffect() {
  static uint8_t heat[MATRIX_HEIGHT][MATRIX_WIDTH];

  for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
      heat[y][x] = 0;
    }
  }

  while (true) {
    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
      for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        uint8_t cooling = random(2, 14 + y * 2);
        heat[y][x] = (heat[y][x] > cooling) ? heat[y][x] - cooling : 0;
      }
    }

    for (uint8_t y = 0; y < MATRIX_HEIGHT - 1; y++) {
      for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        uint8_t leftX = (x == 0) ? 0 : x - 1;
        uint8_t rightX = (x == MATRIX_WIDTH - 1) ? MATRIX_WIDTH - 1 : x + 1;
        uint16_t spread = heat[y + 1][leftX] + heat[y + 1][x] + heat[y + 1][rightX];
        if (y + 2 < MATRIX_HEIGHT) {
          spread += heat[y + 2][x];
          heat[y][x] = spread / 4;
        } else {
          heat[y][x] = spread / 3;
        }
      }
    }

    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
      uint8_t baseHeat = random(175, 256);
      if (random(100) < 28) {
        baseHeat = random(220, 256);
      }
      if (random(100) < 12) {
        baseHeat = random(110, 180);
      }
      heat[MATRIX_HEIGHT - 1][x] = baseHeat;

      if (random(100) < 25) {
        uint8_t boostY = MATRIX_HEIGHT - 2;
        uint16_t boosted = heat[boostY][x] + random(18, 55);
        heat[boostY][x] = (boosted > 255) ? 255 : boosted;
      }
    }

    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
      for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        bool blueAccent = (y == MATRIX_HEIGHT - 1) && (heat[y][x] > 180) && (random(100) < 30);
        pixels.setPixelColor(xyToIndex(x, y), fireColorFromHeat(heat[y][x], y, blueAccent));
      }
    }

    pixels.show();
    delay(45);
    yield();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Version: ");
  Serial.println(version);
  randomSeed(analogRead(A0) + micros());
  pixels.begin();
  wipe();

  lightup(satzneu, foregroundColor);
  pixels.show();
  delay(4000);

  fireEffect();
}

void loop() {

}
