#include <NimBLEDevice.h>

static NimBLEClient* pClient = nullptr;
static bool doConnect = false;
static bool startDiscovery = false;
static NimBLEAdvertisedDevice* targetDevice = nullptr;

void notifyCB(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  Serial.printf("Data [%s] (%d Bytes): ", pChar->getUUID().toString().c_str(), length);
  for (size_t i = 0; i < length; i++) {
    if (pData[i] < 0x10) Serial.print("0");
    Serial.print(pData[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
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