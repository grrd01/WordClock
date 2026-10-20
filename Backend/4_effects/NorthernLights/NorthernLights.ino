/////////////////////////////////////////////
//
// LOLIN (WEMOS) D1 mini Lite (ESP8266) NorthernLights-Effect
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

uint32_t addColors(uint32_t a, uint32_t b) {
  uint16_t r = ((a >> 16) & 0xFF) + ((b >> 16) & 0xFF);
  uint16_t g = ((a >> 8) & 0xFF) + ((b >> 8) & 0xFF);
  uint16_t bl = (a & 0xFF) + (b & 0xFF);

  if (r > 255) r = 255;
  if (g > 255) g = 255;
  if (bl > 255) bl = 255;

  return pixels.Color((uint8_t)r, (uint8_t)g, (uint8_t)bl);
}



// NorthernLights-Effect
void northernLightsEffect() {
  static float phase = 0.0f;

  const uint32_t skyTop = pixels.Color(0, 4, 18);
  const uint32_t skyBottom = pixels.Color(0, 0, 2);
  const uint32_t aqua = pixels.Color(0, 100, 130);
  const uint32_t emerald = pixels.Color(18, 180, 65);
  const uint32_t violet = pixels.Color(70, 18, 125);

  for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
      float nx = x / (float)(MATRIX_WIDTH - 1);
      float ny = y / (float)(MATRIX_HEIGHT - 1);

      float curtainA = 2.2f
        + 1.4f * sinf(nx * 4.8f + phase * 0.060f)
        + 0.8f * sinf(nx * 10.0f - phase * 0.030f);
      float curtainB = 5.0f
        + 1.0f * cosf(nx * 5.5f - phase * 0.050f + 1.2f)
        + 0.6f * sinf(nx * 12.0f + phase * 0.040f);

      float distA = y - curtainA;
      float distB = y - curtainB;
      float bandA = expf(-(distA * distA) / 1.6f);
      float bandB = expf(-(distB * distB) / 3.8f) * 0.55f;

      float upperGlow = clamp01(1.20f - ny * 0.85f);
      float shimmerA = 0.72f + 0.28f * sinf(phase * 0.150f + x * 0.90f - y * 0.70f);
      float shimmerB = 0.78f + 0.22f * cosf(phase * 0.110f - x * 1.10f + y * 0.40f);
      float intensityA = clamp01(bandA * upperGlow * shimmerA);
      float intensityB = clamp01(bandB * (0.95f - ny * 0.25f) * shimmerB);

      float mainMix = 0.5f + 0.5f * sinf(phase * 0.025f + nx * 6.5f - ny * 2.2f);
      float accentMix = 0.5f + 0.5f * cosf(phase * 0.021f - nx * 7.0f + ny * 1.7f);

      uint32_t skyColor = blendColor(skyBottom, skyTop, clamp01(1.0f - ny * 0.80f));
      uint32_t mainColor = blendColor(aqua, emerald, mainMix);
      uint32_t accentColor = blendColor(Blue, violet, accentMix);

      uint32_t color = skyColor;
      color = addColors(color, scaleColor(mainColor, intensityA));
      color = addColors(color, scaleColor(accentColor, intensityB));

      float crest = clamp01((intensityA - 0.72f) / 0.28f) * 0.35f;
      if (crest > 0.0f) {
        color = addColors(color, scaleColor(White, crest));
      }

      uint16_t idx = xyToIndex(x, y);
      pixels.setPixelColor(idx, color);
    }
  }

  pixels.show();
  delay(35);
  phase += 2.0f;
  if (phase > 100000.0f) {
    phase = 0.0f;
  }
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
  delay(2000);
}

void loop() {
  northernLightsEffect();
}
