#include <Arduino.h>
#include <Adafruit_NeoPixel.h>  // v1.10.4

// function definitions
static uint32_t Black = Adafruit_NeoPixel::Color(0, 0, 0);
static uint32_t White = Adafruit_NeoPixel::Color(49, 52, 34);
static uint32_t Green = Adafruit_NeoPixel::Color(10, 90, 0);
static uint32_t Red = Adafruit_NeoPixel::Color(90, 0, 0);
static uint32_t Blue = Adafruit_NeoPixel::Color(0, 20, 85);

int hour = 8;
int minute = 25;
int second = 12;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(121, D7, NEO_GRB + NEO_KHZ800);
uint32_t foregroundColor = White;
uint32_t backgroundColor = Black;

// Sets all pixels to the background
void blank() {
  for (int x = 0; x < pixels.numPixels(); ++x) {
    pixels.setPixelColor(x, backgroundColor);
  }
}

// Sets all pixels to the background and displays it
void wipe() {
  blank();
  pixels.show();
}

// Hilfsfunktion: Berechnet den Index für das Schlangenmuster (Serpentine)
int getIndex(int x, int y) {
  if (x < 0 || x > 10 || y < 0 || y > 10) return -1;
  return (y % 2 == 1) ? (y * 11) + (10 - x) : (y * 11) + x;
}

// Hilfsfunktion zur Sättigungs-Addition (verhindert Farb-Überlauf)
uint8_t qadd8(uint8_t a, uint8_t b) {
  uint16_t res = a + b;
  return (res > 255) ? 255 : (uint8_t)res;
}

// Subpixel-Zeichnen: Nur Pixel direkt auf der Linie werden beleuchtet
void drawThinLineAA(float x1, float y1, float angle, float length, uint8_t r, uint8_t g, uint8_t b) {
  // Wir gehen in sehr kleinen Schritten entlang der Linie
  for (float d = 0; d <= length; d += 0.2) {
    float currX = x1 + cos(angle) * d;
    float currY = y1 + sin(angle) * d;

    int x0 = (int)floor(currX);
    int y0 = (int)floor(currY);
    float x_frac = currX - x0;
    float y_frac = currY - y0;

    // Wir beleuchten nur die 4 engsten Nachbarn mit einer steilen Abfallkurve
    for (int i = 0; i <= 1; i++) {
      for (int j = 0; j <= 1; j++) {
        float weight = (i == 0 ? 1.0 - x_frac : x_frac) * (j == 0 ? 1.0 - y_frac : y_frac);
        
        // Potenzieren des Gewichts macht die Linie dünner (schärferer Kontrast)
        weight = pow(weight, 2); 

        if (weight > 0.1) { // Threshold, damit keine "Geisterpixel" entstehen
          int idx = getIndex(x0 + i, y0 + j);
          if (idx != -1) {
            uint32_t c = pixels.getPixelColor(idx);
            uint8_t nr = qadd8((c >> 16) & 0xFF, r * weight);
            uint8_t ng = qadd8((c >> 8) & 0xFF, g * weight);
            uint8_t nb = qadd8(c & 0xFF, b * weight);
            pixels.setPixelColor(idx, pixels.Color(nr, ng, nb));
          }
        }
      }
    }
  }
}

void analogEffect(int h, int m, int s) {
  pixels.clear();

  float centerX = 5.0;
  float centerY = 5.0;

  // Winkel (12 Uhr ist oben)
  float s_ang = (s * 6.0) * DEG_TO_RAD - HALF_PI;
  float m_ang = (m * 6.0 + s * 0.1) * DEG_TO_RAD - HALF_PI;
  float h_ang = ((h % 12) * 30.0 + m * 0.5) * DEG_TO_RAD - HALF_PI;

  // Stundenzeiger (Weiss, Länge 4) - Helligkeit reduziert für Schärfe
  drawThinLineAA(centerX, centerY, h_ang, 4.0, 180, 180, 180);

  // Minutenzeiger (Grau, Länge 5)
  drawThinLineAA(centerX, centerY, m_ang, 5.0, 80, 80, 80);

  // Sekundenzeiger (Rot, Länge 6)
  drawThinLineAA(centerX, centerY, s_ang, 6.0, 200, 0, 0);

  // Das Zentrum (Pixel 60) bekommt einen fixen Punkt zur Orientierung
  pixels.setPixelColor(60, pixels.Color(50, 50, 50)); 
  
  pixels.show();
}

void setup() {
  Serial.begin(115200);
  Serial.println("");
  pixels.begin();
  wipe();
  pixels.show();
}

void loop() {
  second++;
  if (second >= 60) {
    second = 0;
    minute++;
    if (minute >= 60) {
      minute = 0;
      hour++;
      if (hour >= 24) {
        hour = 0;
      }
    }
  }
  delay(1000);
  analogEffect(hour, minute, second);
}

