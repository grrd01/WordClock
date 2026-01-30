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

struct Drop {
  float y; // aktuelle Position (kann zwischen den Zeilen liegen)
  float speed; // Geschwindigkeit (Zeilen pro Frame)
  uint8_t length; // Abdunklungslänge
  uint8_t maxBrightness; // Maximale Helligkeit (0..255)
  bool active; // Ob dieser Drop aktiv ist
};

Drop drops[11][2]; // 11 Spalten, bis zu 2 Drops pro Spalte
uint16_t dropCooldown[11]; // Cooldown pro Spalte bis zum nächsten neuen Drop
uint32_t frameCounter = 0; // Frame-Zähler für 4-Sekunden-Limit

bool isInSatzNeu(uint16_t pixelIdx) {
  for (int x = 0; x < pixels.numPixels() + 1; x++) {
    if (satzneu[x] == -1) break;
    if (satzneu[x] == pixelIdx) return true;
  }
  return false;
}

void initDrop(uint8_t col, uint8_t dropIndex) {
  drops[col][dropIndex].y = -random(1, 8); // Startet oberhalb der Matrix
  drops[col][dropIndex].speed = 0.1 + random(5, 80) / 100.0; // 0.15..0.90
  drops[col][dropIndex].length = random(3, 8); // 3..7
  drops[col][dropIndex].maxBrightness = random(120, 255); // 120..255
  drops[col][dropIndex].active = true;
}

void matrixEffect() {
  frameCounter = 0;
  for (uint8_t x = 0; x < 11; x++) {
    initDrop(x, 0);
    dropCooldown[x] = 0;
  }
  while (true) {
    frameCounter++;
    bool allowNewDrops = (frameCounter < 133); // 4 Sekunden / 30ms = ~133 Frames
    
    bool hasActiveDrops = false;
    
    for (uint8_t x = 0; x < 11; x++) {
      // Cooldown für neue Drops verwalten
      if (dropCooldown[x] > 0) {
        dropCooldown[x]--;
      }
      
      // Alle Drops in dieser Spalte verarbeiten
      for (uint8_t d = 0; d < 2; d++) {
        if (!drops[x][d].active) {
          // Wenn dieser Drop inaktiv ist und Cooldown abgelaufen, neuen Drop starten
          if (dropCooldown[x] == 0 && allowNewDrops) {
            initDrop(x, d);
            dropCooldown[x] = random(5, 30); // Kurzer Cooldown bis zum nächsten Drop
          }
          continue;
        }
        
        hasActiveDrops = true;
        drops[x][d].y += drops[x][d].speed;
        
        // Drop ist fertig, wenn er unten angekommen ist
        if (drops[x][d].y - drops[x][d].length > 11) {
          drops[x][d].active = false;
          continue;
        }
        
        // Zeichne den Drop
        for (int8_t i = 0; i <= drops[x][d].length; i++) {
          int8_t py = (int8_t)(drops[x][d].y) - i;
          if (py < 0 || py > 11) continue;
          uint16_t idx = xyToIndex(x, py);
          
          // Prüfe, ob dieser Pixel in satzneu enthalten ist
          if (isInSatzNeu(idx)) {
            // Satzneu-Pixel: keine Fade, volle Helligkeit
            pixels.setPixelColor(idx, foregroundColor);
          } else {
            // Andere Pixel: normales Faden
            float fade = 1.0 - (float)i / drops[x][d].length;
            fade = fade * fade; // Exponentieller Fade für stärkeren Effekt
            uint8_t brightness = (uint8_t)(drops[x][d].maxBrightness * fade);
            uint32_t col = pixels.Color(
              (uint8_t)((foregroundColor >> 16) & 0xFF) * brightness / 255,
              (uint8_t)((foregroundColor >> 8) & 0xFF) * brightness / 255,
              (uint8_t)(foregroundColor & 0xFF) * brightness / 255
            );
            pixels.setPixelColor(idx, col);
          }
        }
      }
    }
    
    pixels.show();
    delay(30);
    
    // Schleife nur beenden wenn keine Drops mehr aktiv sind und neue Drops nicht mehr gespawnt werden
    if (!allowNewDrops && !hasActiveDrops) break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Version: ");
  Serial.println(version);
  pixels.begin();
  wipe();

  lightup(satzalt, foregroundColor);
  pixels.show();
  delay(4000);


  
  matrixEffect();
}

void loop() {
  // Matrix-Effekt läuft in setup()
}
