/////////////////////////////////////////////
//
// LOLIN (WEMOS) D1 mini Lite (ESP8266) Thunderstorm-Effect
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

uint32_t scaleColor(uint32_t color, uint8_t brightness) {
  return pixels.Color(
    (uint8_t)((((color >> 16) & 0xFF) * brightness) / 255),
    (uint8_t)((((color >> 8) & 0xFF) * brightness) / 255),
    (uint8_t)(((color & 0xFF) * brightness) / 255)
  );
}

void setSky(uint32_t color) {
  for (uint16_t i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, color);
  }
}

void drawPixelSafe(int8_t x, int8_t y, uint32_t color) {
  if (x < 0 || x > 10 || y < 0 || y > 10) {
    return;
  }

  pixels.setPixelColor(xyToIndex((uint8_t)x, (uint8_t)y), color);
}

void flashSky(uint8_t brightness, uint16_t holdMs) {
  const uint8_t fadeSteps = 8;
  const uint16_t fadeStepMs = (uint16_t)random(12, 46);
  const uint32_t skyFlashColor = pixels.Color(24, 42, 100);
  uint8_t pixelBrightness[121];
  int8_t cloudVariation[4][4];

  for (uint8_t cloudY = 0; cloudY < 4; cloudY++) {
    for (uint8_t cloudX = 0; cloudX < 4; cloudX++) {
      cloudVariation[cloudY][cloudX] = (int8_t)random(-75, 76);
    }
  }

  for (uint8_t y = 0; y < 11; y++) {
    uint16_t rowBase = 20 + (uint16_t)(10 - y) * 23;
    for (uint8_t x = 0; x < 11; x++) {
      int16_t variation = cloudVariation[y / 3][x / 3] + random(-18, 19);
      int16_t cloudLevel = constrain((int16_t)rowBase + variation, 0, 255);
      pixelBrightness[xyToIndex(x, y)] = (uint8_t)cloudLevel;
    }
  }

  for (uint8_t step = 1; step <= fadeSteps; step++) {
    uint8_t frameBrightness = (uint16_t)brightness * step / fadeSteps;
    for (uint16_t i = 0; i < pixels.numPixels(); i++) {
      uint8_t pixelLevel = (uint16_t)frameBrightness * pixelBrightness[i] / 255;
      pixels.setPixelColor(i, scaleColor(skyFlashColor, pixelLevel));
    }
    pixels.show();
    delay(fadeStepMs);
  }

  delay(holdMs);

  for (int8_t step = fadeSteps - 1; step >= 0; step--) {
    uint8_t frameBrightness = (uint16_t)brightness * step / fadeSteps;
    for (uint16_t i = 0; i < pixels.numPixels(); i++) {
      uint8_t pixelLevel = (uint16_t)frameBrightness * pixelBrightness[i] / 255;
      pixels.setPixelColor(i, scaleColor(skyFlashColor, pixelLevel));
    }
    pixels.show();
    delay(fadeStepMs);
  }
}

void generateBoltPath(int8_t *boltX, int8_t *branchDir, uint8_t *branchLen) {
  int8_t currentX = random(2, 9);

  for (uint8_t y = 0; y < 11; y++) {
    currentX = (int8_t)constrain(currentX + random(-1, 2), 0, 10);
    if (y > 1 && y < 9 && random(0, 100) < 18) {
      currentX = (int8_t)constrain(currentX + random(-1, 2), 0, 10);
    }

    boltX[y] = currentX;
    branchDir[y] = 0;
    branchLen[y] = 0;

    if (y > 1 && y < 9 && random(0, 100) < 20) {
      branchDir[y] = random(0, 2) == 0 ? -1 : 1;
      branchLen[y] = (uint8_t)random(1, 3);
    }
  }
}

void drawBoltFrame(const int8_t *boltX, const int8_t *branchDir, const uint8_t *branchLen, uint8_t visibleRows) {
  const uint32_t boltCore = pixels.Color(78, 82, 56);
  const uint32_t boltGlow = scaleColor(White, 145);
  const uint32_t faintGlow = scaleColor(White, 70);

  blank();

  for (uint8_t y = 0; y <= visibleRows && y < 11; y++) {
    int8_t x = boltX[y];

    drawPixelSafe(x, y, boltCore);

    if (y > 0 && boltX[y - 1] != x) {
      drawPixelSafe(boltX[y - 1], y, faintGlow);
    }

    if (branchLen[y] > 0) {
      for (uint8_t step = 0; step < branchLen[y]; step++) {
        int8_t branchX = (int8_t)constrain(x + branchDir[y] * (step + 1), 0, 10);
        int8_t branchY = y + step + 1;
        if (branchY > (int8_t)visibleRows || branchY > 10) {
          break;
        }

        drawPixelSafe(branchX, branchY, step == 0 ? boltGlow : faintGlow);
      }
    }
  }

  pixels.show();
}

// Thunderstorm-Effect
void thunderstormEffect() {
  int8_t boltX[11];
  int8_t branchDir[11];
  uint8_t branchLen[11];

  wipe();

  while (true) {
    delay(random(160, 900));

    uint8_t strikeCount = (uint8_t)random(1, 5);
    for (uint8_t strike = 0; strike < strikeCount; strike++) {
      bool flashWholeSky = random(0, 100) < 22;
      bool doubleStrike = random(0, 100) < 30;

      if (flashWholeSky) {
        uint8_t flashBrightness = random(0, 100) < 25
          ? (uint8_t)random(35, 120)
          : (uint8_t)random(130, 231);
        flashSky(flashBrightness, (uint16_t)random(18, 50));
      }

      generateBoltPath(boltX, branchDir, branchLen);
      for (uint8_t row = 0; row < 11; row++) {
        drawBoltFrame(boltX, branchDir, branchLen, row);
        delay((uint16_t)random(10, 22));
      }

      delay((uint16_t)random(120, 220));

      if (doubleStrike) {
        flashSky((uint8_t)random(45, 151), (uint16_t)random(18, 40));
        drawBoltFrame(boltX, branchDir, branchLen, 10);
        delay((uint16_t)random(120, 220));
      }

      uint8_t afterglowCount = random(0, 100) < 35 ? (uint8_t)random(1, 4) : 0;
      for (uint8_t flash = 0; flash < afterglowCount; flash++) {
        flashSky((uint8_t)random(8, 151), (uint16_t)random(5, 101));
        if (flash + 1 < afterglowCount) {
          delay((uint16_t)random(45, 120));
        }
      }

      wipe();
      delay((uint16_t)random(30, 140));
    }
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

  thunderstormEffect();
}

void loop() {

}
