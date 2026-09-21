/*
 * ==================================================================================
 * ShanWan Q36 HID-Mode Decoder (Für "Q36 for Android" Modus)
 * Controller im D-Modus pairen
 * ==================================================================================
 */
#include <NimBLEDevice.h>

static NimBLEClient* pClient = nullptr;
static bool doConnect = false;
static bool startDiscovery = false;
static NimBLEAdvertisedDevice* targetDevice = nullptr;

void notifyCB(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  if (length < 10) return;

  uint8_t dpad    = pData[4]; // 5. Byte
  uint8_t btnMain = pData[5]; // 6. Byte
  uint8_t btnSub  = pData[6]; // 7. Byte

  // Statische Variablen behalten ihren Wert zwischen den Aufrufen
  static uint8_t lastDpad   = 0xFF;
  static uint8_t lastMain   = 0x00;
  static uint8_t lastSub    = 0x00;

  // Wenn sich der Zustand seit dem letzten Paket nicht veraendert hat: Abbrechen
  if (dpad == lastDpad && btnMain == lastMain && btnSub == lastSub) {
    return;
  }

  // Aktuellen Zustand fuer den naechsten Vergleich speichern
  lastDpad = dpad;
  lastMain = btnMain;
  lastSub  = btnSub;

  // 1. Tasten losgelassen
  if (dpad == 0xFF && btnMain == 0x00 && btnSub == 0x00) {
    Serial.println("Taste losgelassen");
    return;
  }

  // 2. Steuerkreuz (D-Pad)
  if (dpad != 0xFF) {
    switch (dpad) {
      case 0x00: Serial.println("Taste UP gedrueckt"); return;
      case 0x02: Serial.println("Taste RIGHT gedrueckt"); return;
      case 0x04: Serial.println("Taste DOWN gedrueckt"); return;
      case 0x06: Serial.println("Taste LEFT gedrueckt"); return;
    }
  }

  // 3. Haupt-Buttons (A, B, X, Y, L, R)
  switch (btnMain) {
    case 0x01: Serial.println("Taste A gedrueckt"); return;
    case 0x02: Serial.println("Taste B gedrueckt"); return;
    case 0x08: Serial.println("Taste X gedrueckt"); return;
    case 0x10: Serial.println("Taste Y gedrueckt"); return;
    case 0x40: Serial.println("Taste L gedrueckt"); return;
    case 0x80: Serial.println("Taste R gedrueckt"); return;
  }

  // 4. Schulter- & Zusatz-Buttons (L2, R2, SL, SR)
  switch (btnSub) {
    case 0x01: Serial.println("Taste L2 gedrueckt"); return;
    case 0x02: Serial.println("Taste R2 gedrueckt"); return;
    case 0x04: Serial.println("Taste SL gedrueckt"); return;
    case 0x08: Serial.println("Taste SR gedrueckt"); return;
  }

  // Falls eine unerkannte Kombination gedrueckt wird
  Serial.printf("Unbekannt: [4]=0x%02X [5]=0x%02X [6]=0x%02X\n", dpad, btnMain, btnSub);
}

class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) override {
    Serial.println(">> Verbunden!");
  }

  void onDisconnect(NimBLEClient* pClient, int reason) override {
    Serial.printf(">> Verbindung getrennt! Reason: %d\n", reason);
    startDiscovery = false;
    NimBLEDevice::getScan()->start(0, false);
  }

  void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
    if (connInfo.isEncrypted()) {
      Serial.println(">> Security/Pairing ERFOLGREICH! Starte Service-Discovery...");
      startDiscovery = true; // Signalisiere Hauptschleife: Jetzt sicher abfragen!
    } else {
      Serial.println(">> Security/Pairing FEHLGESCHLAGEN!");
    }
  }
};

class ScanCallbacks : public NimBLEScanCallbacks {
  void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) override {
    if (advertisedDevice->getName().find("ShanWan") != std::string::npos ||
        advertisedDevice->getName().find("Q36") != std::string::npos) {
      
      Serial.printf("Controller gefunden: %s [%s]\n", 
                    advertisedDevice->getName().c_str(), 
                    advertisedDevice->getAddress().toString().c_str());
      
      NimBLEDevice::getScan()->stop();
      targetDevice = const_cast<NimBLEAdvertisedDevice*>(advertisedDevice);
      doConnect = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Suche ShanWan Q36 Controller...");

  NimBLEDevice::init("ESP32C6_HID_Host");
  
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(new ScanCallbacks());
  pScan->setActiveScan(true);
  pScan->start(0, false);
}

void loop() {
  if (doConnect) {
    doConnect = false;
    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new ClientCallbacks());

    if (pClient->connect(targetDevice)) {
      Serial.println(">> Starte Encryption...");
      NimBLEDevice::startSecurity(pClient->getConnHandle());
    } else {
      Serial.println(">> Verbindung fehlgeschlagen.");
      NimBLEDevice::getScan()->start(0, false);
    }
  }

  // Erst ausführen, WENN onAuthenticationComplete() ERFOLGREICH meldet!
  if (startDiscovery) {
    startDiscovery = false;
    
    // Kurze Pause, damit der BLE-Stack nach Key-Exchange bereit ist
    delay(500);

    int totalSubscribed = 0;

    for (auto pService : pClient->getServices(true)) {
      Serial.printf("Durchsuche Service: %s\n", pService->getUUID().toString().c_str());

      for (auto pChar : pService->getCharacteristics(true)) {
        // Prüfe ob Characteristic Benachrichtigungen senden kann
        if (pChar->canNotify() || pChar->canIndicate()) {
          // Explizit auf Subscriben mit Antwort erzwingen
          if (pChar->subscribe(true, notifyCB)) {
            totalSubscribed++;
            Serial.printf("   --> ERFOLG: Subscribed auf Char: %s\n", pChar->getUUID().toString().c_str());
          } else {
            Serial.printf("   --> FEHLER beim Subscriben auf Char: %s\n", pChar->getUUID().toString().c_str());
          }
        }
      }
    }

    Serial.printf(">> Fertig! Insgesamt auf %d Kanaele subscribed.\n", totalSubscribed);
  }

  delay(10);
}