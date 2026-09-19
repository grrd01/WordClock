/*
 * ==================================================================================
 * ShanWan Q36 Controller native BLE-Connection (Timer-Release Version)
 * ==================================================================================
 * Configuration:
 * - Board: Seeed Studio XIAO ESP32C6
 * - Controller: ShanWan Q36
 * Pair Controller in V-Mode (ShootingPlus for Android)
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Die beiden UUIDs für die beiden Datenkanäle
static BLEUUID inputCharUUID((uint16_t)0x2A4D); 
static BLEUUID customCharUUID("94990003-1111-6666-8888-0123456789ab"); // Für Select, Start, Home, R

static boolean doConnect = false;
static boolean connected = false;
static BLEAdvertisedDevice* myDevice = nullptr;

// Software-Zustandsspeicher für Taste Y
static bool yIstGedrueckt = false; 

// Globale Variablen für die zeitbasierte Freigabe (Select & Start)
volatile bool globalSelectGedrueckt = false;
volatile bool globalStartGedrueckt = false;
volatile unsigned long globalSelectTimer = 0;
volatile unsigned long globalStartTimer = 0;
const unsigned long globalTimeoutMs = 150; // Zeitfenster in ms, nach dem die Taste als losgelassen gilt

// Sicherheits-Callback: Bestätigt dem Controller die Kopplungsanfrage
class MySecurityCallbacks : public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() {
    Serial.println("-> Security: Passkey angefordert (000000)");
    return 0;
  }
  
  void onPassKeyNotify(uint32_t pass_key) {
    Serial.printf("-> Security: Passkey erhalten: %d\n", pass_key);
  }
  
  bool onSecurityRequest() {
    Serial.println("-> Security: Verbindungssicherheit angefordert.");
    return true;
  }
  
  bool onConnectConfirm() {
    Serial.println("-> Security: Bestätige Verbindungsanfrage.");
    return true;
  }
};

class MyClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("-> Physisch mit Q36 verbunden!");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("-> Verbindung zum Q36 verloren.");
  }
};

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (length < 1) return;

    // UUID der aktuellen Charakteristik als String auslesen und klein schreiben
    String charUUID = pBLERemoteCharacteristic->getUUID().toString().c_str();
    charUUID.toLowerCase();

    // ==============================================================================
    // 1. DATEN VOM CUSTOM-KANAL (Select, Start, Home, R...)
    // ==============================================================================
    if (charUUID.indexOf("94990003") >= 0) {
        if (length < 8) return; 

        uint8_t tastenByte = pData[7]; 
        unsigned long jetzt = millis();

        // --- SELECT GEDRÜCKT ---
        if (tastenByte == 0x88) {
            globalSelectTimer = jetzt; // Stoppuhr bei jedem empfangenen Paket zurücksetzen
            if (!globalSelectGedrueckt) {
                globalSelectGedrueckt = true;
                Serial.println("SELECT GEDRÜCKT");
            }
        }

        // --- START GEDRÜCKT ---
        if (tastenByte == 0x8A) {
            globalStartTimer = jetzt; // Stoppuhr bei jedem empfangenen Paket zurücksetzen
            if (!globalStartGedrueckt) {
                globalStartGedrueckt = true;
                Serial.println("START GEDRÜCKT");
            }
        }
        return; // Callback hier beenden
    }

    // ==============================================================================
    // 2. DATEN VOM STANDARD-HID-KANAL (A, B, X, Y, D-Pad...)
    // ==============================================================================
    if (length < 10) return;

    static uint8_t lastData[17] = {0};
    bool changed = false;

    for (size_t i = 0; i < length; i++) {
        if (pData[i] != lastData[i]) {
            changed = true;
            break;
        }
    }

    if (changed) {
        // Rohdaten ausgeben
        Serial.print("ROHDATEN: ");
        for (size_t i = 0; i < length; i++) {
            Serial.printf("%02X ", pData[i]);
        }
        Serial.println();

        // Tastenerkennung durchführen
        uint8_t statusByte = pData[0];        // Erstes Byte (0x03 = gedrückt, 0x02 = losgelassen)
        uint8_t keyByte = pData[3];           // Viertes Byte (Index 3) für die Tasten-ID
        uint8_t subByte = pData[2] & 0x0F;    // Zweite Stelle des dritten Bytes (Index 2)

        // Erkennung für Taste A (Wertebereich 0xEE bis 0xF0 und Sub 04)
        if (keyByte >= 0xEE && keyByte <= 0xF0 && subByte == 0x04) {
            if (statusByte == 0x03) {
                Serial.println("A GEDRÜCKT");
            } else if (statusByte == 0x02) {
                Serial.println("A LOSGELASSEN");
            }
        }
        // Erkennung für Taste B (Wertebereich 0xD5 bis 0xD7)
        else if (keyByte >= 0xD5 && keyByte <= 0xD7) {
            if (statusByte == 0x03) {
                Serial.println("B GEDRÜCKT");
            } else if (statusByte == 0x02) {
                Serial.println("B LOSGELASSEN");
            }
        }
        // Erkennung für Taste X (Wertebereich 0xEC bis 0xEE und Sub 01)
        else if (keyByte >= 0xEC && keyByte <= 0xEE && subByte == 0x01) {
            if (statusByte == 0x03) {
                Serial.println("X GEDRÜCKT");
            } else if (statusByte == 0x02) {
                Serial.println("X LOSGELASSEN");
            }
        }
        // Erkennung für Taste Y (Wertebereich 0xF3 bis 0xF5 und Sub 0D)
        else if (keyByte >= 0xF3 && keyByte <= 0xF5 && subByte == 0x0D) {
            if (statusByte == 0x03) {
                if (!yIstGedrueckt) {
                    yIstGedrueckt = true;
                    Serial.println("Y GEDRÜCKT");
                } else {
                    yIstGedrueckt = false;
                    Serial.println("Y LOSGELASSEN");
                }
            }
        }
        // Erkennung für Taste left (Wertebereich 0x70 bis 0x72)
        else if (keyByte >= 0x70 && keyByte <= 0x72) {
            if (statusByte == 0x03) {
                Serial.println("left GEDRÜCKT");
            } else if (statusByte == 0x02) {
                Serial.println("left LOSGELASSEN");
            }
        }
        // Erkennung für Taste right (Wertebereich 0x92 bis 0x94)
        else if (keyByte >= 0x92 && keyByte <= 0x94) {
            if (statusByte == 0x03) {
                Serial.println("right GEDRÜCKT");
            } else if (statusByte == 0x02) {
                Serial.println("right LOSGELASSEN");
            }
        }
        // Erkennung für Taste up (Wertebereich 0xAA bis 0xAC)
        else if (keyByte >= 0xAA && keyByte <= 0xAC) {
            if (statusByte == 0x03) {
                Serial.println("up GEDRÜCKT");
            } else if (statusByte == 0x02) {
                Serial.println("up LOSGELASSEN");
            }
        }
        // Erkennung für Taste down (Wertebereich 0xAD bis 0xAF)
        else if (keyByte >= 0xAD && keyByte <= 0xAF) {
            if (statusByte == 0x03) {
                Serial.println("down GEDRÜCKT");
            } else if (statusByte == 0x02) {
                Serial.println("down LOSGELASSEN");
            }
        }
        // Erkennung für Taste L (Wertebereich 0xC7 bis 0xC9)
        else if (keyByte >= 0xC7 && keyByte <= 0xC9) {
            if (statusByte == 0x03) {
                Serial.println("L GEDRÜCKT");
            } else if (statusByte == 0x02) {
                Serial.println("L LOSGELASSEN");
            }
        }
        // Erkennung für Taste L2 (Wertebereich 0xEF bis 0xF1 und Sub 06)
        else if (keyByte >= 0xEF && keyByte <= 0xF1 && subByte == 0x06) {
            if (statusByte == 0x03) {
                Serial.println("L2 GEDRÜCKT");
            } else if (statusByte == 0x02) {
                Serial.println("L2 LOSGELASSEN");
            }
        }
        // Erkennung für Taste R2 (Wertebereich 0x16 bis 0x18)
        else if (keyByte >= 0x16 && keyByte <= 0x18) {
            if (statusByte == 0x03) {
                Serial.println("R2 GEDRÜCKT");
            } else if (statusByte == 0x02) {
                Serial.println("R2 LOSGELASSEN");
            }
        }

        for (size_t i = 0; i < length; i++) {
            lastData[i] = pData[i];
        }
    }
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) { 
    String name = advertisedDevice.getName().c_str();
    name.toLowerCase();
    
    if (name.indexOf("q36") >= 0 || name.indexOf("shanwan") >= 0) {
      Serial.println("\n==================================================");
      Serial.printf("GAMEPAD GEFUNDEN: '%s'\n", advertisedDevice.getName().c_str());
      Serial.println("==================================================");
      
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

bool connectToServer() {
    Serial.printf("Verbinde mit Q36 unter Adresse: %s\n", myDevice->getAddress().toString().c_str());
    
    BLEClient* pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallbacks());

    if (!pClient->connect(myDevice)) {
        Serial.println("Fehler: Physische Verbindung fehlgeschlagen.");
        delete myDevice;
        myDevice = nullptr;
        return false;
    }
    
    Serial.println("Verbunden! Erhöre MTU...");
    pClient->setMTU(40); 
    
    Serial.println("Werte kurz auf Verschlüsselungs-Handshake...");
    delay(2000);

    Serial.println("Analysiere Services...");
    auto services = pClient->getServices();
    int activeNotifications = 0;

    for (auto servicePair : *services) {
        BLERemoteService* pService = servicePair.second;
        auto characteristics = pService->getCharacteristics();

        for (auto charPair : *characteristics) {
            BLERemoteCharacteristic* pChara = charPair.second;
            BLEUUID uuid = pChara->getUUID();

            if (uuid.equals(inputCharUUID) || uuid.equals(customCharUUID)) {
                Serial.printf("-> Kanal gefunden: %s (Handle: 0x%02X). ", 
                              uuid.toString().c_str(), pChara->getHandle());
                
                if (pChara->canNotify()) {
                    pChara->registerForNotify(notifyCallback);
                    
                    BLERemoteDescriptor* pDec = pChara->getDescriptor(BLEUUID((uint16_t)0x2902));
                    if (pDec != nullptr) {
                        uint8_t val[] = {0x01, 0x00};
                        pDec->writeValue(val, 2, true);
                        Serial.print("Descriptor beschrieben! ");
                    }
                    Serial.println("Lauschen aktiviert! [OK]");
                    activeNotifications++;
                } else {
                    Serial.println("Unterstützt keine Live-Daten. [Skip]");
                }
            }
        }
    }

    if (activeNotifications > 0) {
        Serial.println("\n>>> ERFOLG! Auf allen Kanälen aktiv. Drücke jetzt Knöpfe am Q36!");
        connected = true;
    } else {
        Serial.println("Fehler: Keine aktiven Datenkanäle gefunden.");
        pClient->disconnect();
    }

    delete myDevice;
    myDevice = nullptr;
    return connected;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starte XIAO ESP32-C6 (Secure Connection Mode)...");
  
  BLEDevice::init("XIAO-C6-Host");
  BLEDevice::setSecurityCallbacks(new MySecurityCallbacks());

  BLESecurity security;
  security.setAuthenticationMode(ESP_LE_AUTH_BOND);
  security.setCapability(ESP_IO_CAP_NONE);

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(15, false);
}

void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("Gamepad im Überwachungsmodus.");
    } else {
      Serial.println("Verbindung fehlgeschlagen. Starte neuen Scan...");
      BLEDevice::getScan()->start(15, false);
    }
    doConnect = false;
  }

  // ==============================================================================
  // HINTERGRUND-TIMER FÜR DAS AUTOMATISCHE LOSLASSEN
  // ==============================================================================
  unsigned long jetzt = millis();
  
  // Wenn SELECT als gedrückt gilt, aber seit 150ms keine Daten mehr kamen
  if (globalSelectGedrueckt && (jetzt - globalSelectTimer > globalTimeoutMs)) {
      globalSelectGedrueckt = false;
      Serial.println("SELECT LOSGELASSEN");
  }
  
  // Wenn START als gedrückt gilt, aber seit 150ms keine Daten mehr kamen
  if (globalStartGedrueckt && (jetzt - globalStartTimer > globalTimeoutMs)) {
      globalStartGedrueckt = false;
      Serial.println("START LOSGELASSEN");
  }

  delay(1); // Extrem kurzes Delay, damit der Loop blitzschnell reagiert
}