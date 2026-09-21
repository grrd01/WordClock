/*
 * ==================================================================================
 * BH1750 Brithness Sensor an Wemos D1 mini
 * Verbindungsschema:
 * - 3.3V           -> VCC
 * - D1/GPIO5/SCL   -> SCL
 * - D2/GPIO4/SDA   -> SDA
 * - GND/G          -> GND
 * - ADDR nicht verwendet
 * ==================================================================================
 */

#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);     // SDA, SCL
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  Serial.println("BH1750 gestartet");
}

void loop() {
  float lux = lightMeter.readLightLevel();
  Serial.print("Helligkeit: ");
  Serial.print(lux);
  Serial.println(" lx");
  delay(1000);
}