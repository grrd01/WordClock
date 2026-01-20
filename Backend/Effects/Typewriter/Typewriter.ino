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

static int WordEs[] = {0, 1, -1};
static int WordIst[] = {3, 4, 5, 6, -1};
static int WordHalb[] = {39, 38, 37, 36, 35, -1};

static int WordFix[] = {77, 98, 99, -1};
static int WordWifi[] = {120, -1};
static int WordNach[] = {42, 41, -1};
static int WordVor[] = {30, 31, 32, -1};

static int SymbolWifi[] = {120, -1};

static int *WordURL[] = {WordFix, WordWifi};

// Stunde
static int WordStundeEins[] = {44, 45, 46, -1};                     // EIS
static int WordStundeZwei[] = {48, 49, 50, 51, -1};                 // ZWÖI
static int WordStundeDrei[] = {52, 53, 54, -1};                     // DRÜ
static int WordStundeVier[] = {64, 63, 62, 61, 60, -1};             // VIERI
static int WordStundeFuenf[] = {59, 58, 57, 56, -1};                // FÜFI
static int WordStundeSechs[] = {68, 69, 70, 71, 72, 73, -1};        // SÄCHSI
static int WordStundeSieben[] = {72, 73, 74, 75, 76, -1};           // SIBNI
static int WordStundeAcht[] = {87, 86, 85, 84, 83, -1};             // ACHTI
static int WordStundeNeun[] = {82, 81, 80, 79, -1};                 // NÜNI
static int WordStundeZehn[] = {90, 91, 92, 93, -1};                 // ZÄNI
static int WordStundeElf[] = {95, 96, 97, 98, -1};                  // EUFI
static int WordStundeZwoelf[] = {106, 105, 104, 103, 102, 101, -1}; // ZWÖUFI

static int WordMinFuenf[] = {8, 9, 10, -1};                   // FÜF
static int WordMinZehn[] = {14, 13, 12, -1};                  // ZÄÄ
static int WordMinViertel[] = {21, 20, 19, 18, 17, 16, -1};   // VIERTU
static int WordMinZwanzig[] = {22, 23, 24, 25, 26, 27, -1};   // ZWÄNZG
static int WordMinTicks[] = {113, 114, 116, 117, -1};         // ** **


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

static int WordEs[] = {0, 1, -1};
static int WordIst[] = {3, 4, 5, 6, -1};
static int WordHalb[] = {39, 38, 37, 36, 35, -1};

static int WordFix[] = {77, 98, 99, -1};
static int WordWifi[] = {120, -1};
static int WordNach[] = {42, 41, -1};
static int WordVor[] = {30, 31, 32, -1};

static int SymbolWifi[] = {120, -1};

static int *WordURL[] = {WordFix, WordWifi};

// Stunde
static int WordStundeEins[] = {44, 45, 46, -1};                     // EIS
static int WordStundeZwei[] = {48, 49, 50, 51, -1};                 // ZWÖI
static int WordStundeDrei[] = {52, 53, 54, -1};                     // DRÜ
static int WordStundeVier[] = {64, 63, 62, 61, 60, -1};             // VIERI
static int WordStundeFuenf[] = {59, 58, 57, 56, -1};                // FÜFI
static int WordStundeSechs[] = {68, 69, 70, 71, 72, 73, -1};        // SÄCHSI
static int WordStundeSieben[] = {72, 73, 74, 75, 76, -1};           // SIBNI
static int WordStundeAcht[] = {87, 86, 85, 84, 83, -1};             // ACHTI
static int WordStundeNeun[] = {82, 81, 80, 79, -1};                 // NÜNI
static int WordStundeZehn[] = {90, 91, 92, 93, -1};                 // ZÄNI
static int WordStundeElf[] = {95, 96, 97, 98, -1};                  // EUFI
static int WordStundeZwoelf[] = {106, 105, 104, 103, 102, 101, -1}; // ZWÖUFI

static int WordMinFuenf[] = {8, 9, 10, -1};                   // FÜF
static int WordMinZehn[] = {14, 13, 12, -1};                  // ZÄÄ
static int WordMinViertel[] = {21, 20, 19, 18, 17, 16, -1};   // VIERTU
static int WordMinZwanzig[] = {22, 23, 24, 25, 26, 27, -1};   // ZWÄNZG
static int WordMinTicks[] = {113, 114, 116, 117, -1};         // ** **


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

// satzalt und satzneu Definitionen
int satzalt[] = {0, 1, 3, 4, 5, 6, 8, 9, 10, 30, 31, 32, 39, 38, 37, 36, 35, 68, 69, 70, 71, 72, 73, -1};
int satzneu[] = {0, 1, 3, 4, 5, 6, 22, 23, 24, 25, 26, 27, 42, 41, 59, 58, 57, 56, -1};

// Hilfsfunktion: Prüft, ob LED in Array enthalten ist
bool inArray(int led, int *arr) {
  for (int i = 0; arr[i] != -1; i++) {
    if (arr[i] == led) return true;
  }
  return false;
}

// Hilfsfunktion: Dimm-Animation auf Schwarz
void dimToBlack(int led, uint32_t color, int steps, int delayMs) {
  for (int i = steps; i >= 0; i--) {
    float f = (float)i / steps;
    uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * f);
    uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * f);
    uint8_t b = (uint8_t)((color & 0xFF) * f);
    pixels.setPixelColor(led, pixels.Color(r, g, b));
    pixels.show();
    delay(delayMs);
  }
  pixels.setPixelColor(led, Black);
  pixels.show();
}

// Hilfsfunktion: Lichtimpuls-Animation
void pulseOn(int led, uint32_t color, int steps, int delayMs) {
  // Impuls heller als foregroundColor
  for (int i = 0; i <= steps; i++) {
    float f = 1.0 + 0.5 * sin(PI * i / steps); // von 1.0 bis 1.5 und zurück
    uint8_t r = min(255, (uint8_t)(((color >> 16) & 0xFF) * f));
    uint8_t g = min(255, (uint8_t)(((color >> 8) & 0xFF) * f));
    uint8_t b = min(255, (uint8_t)((color & 0xFF) * f));
    pixels.setPixelColor(led, pixels.Color(r, g, b));
    pixels.show();
    delay(delayMs);
  }
  pixels.setPixelColor(led, color);
  pixels.show();
}

void loop() {
  // 1. Löschen: alle LEDs, die nur in satzalt sind
  for (int i = 120; i >= 0; i--) {
    if (inArray(i, satzalt) && !inArray(i, satzneu)) {
      dimToBlack(i, foregroundColor, 8, 20);
    }
  }
  delay(200);
  // 2. Einschalten: alle LEDs, die nur in satzneu sind
  for (uint8_t y = 0; y < 11; y++) {
    for (uint8_t x = 0; x < 11; x++) {
      int idx = xyToIndex(x, y);
      if (inArray(idx, satzneu) && !inArray(idx, satzalt)) {
        pulseOn(idx, foregroundColor, 8, 20);
      }
    }
  }
  delay(1000); // Animation pausieren
}
