#include <NimBLEDevice.h>

static NimBLEUUID hidServiceUUID((uint16_t)0x1812);
static NimBLEUUID reportCharUUID((uint16_t)0x2A4D);

static NimBLEClient* pClient = nullptr;
static bool doConnect = false;
static NimBLEAdvertisedDevice* targetDevice = nullptr;

class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) override {
    Serial.println(">> Verbunden!");
  }

  void onDisconnect(NimBLEClient* pClient, int reason) override {
    Serial.printf(">> Verbindung getrennt! Reason: %d\n", reason);
    NimBLEDevice::getScan()->start(0, false);
  }

  void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
    if (connInfo.isEncrypted()) {
      Serial.println(">> Security/Pairing ERFOLGREICH!");
    } else {
      Serial.println(">> Security/Pairing FEHLGESCHLAGEN!");
    }
  }
};

void notifyCB(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  Serial.print("Data [");
  Serial.print(length);
  Serial.print("]: ");
  for (size_t i = 0; i < length; i++) {
    if (pData[i] < 0x10) Serial.print("0");
    Serial.print(pData[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

class ScanCallbacks : public NimBLEScanCallbacks {
  void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) override {
    if (advertisedDevice->isAdvertisingService(hidServiceUUID) || 
        advertisedDevice->getName().find("ShanWan") != std::string::npos ||
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
  
  // Bonding und No Input/Output für BLE HID Gamepad
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
      
      // NimBLE v2.x: Security über ConnHandle starten
      NimBLEDevice::startSecurity(pClient->getConnHandle());

      int retry = 0;
      while (!pClient->getConnInfo().isEncrypted() && retry < 30) {
        delay(200);
        retry++;
      }

      BLERemoteService* pService = pClient->getService(hidServiceUUID);
      if (pService) {
        for (auto pChar : pService->getCharacteristics()) {
          if (pChar->getUUID().equals(reportCharUUID)) {
            if (pChar->canNotify() || pChar->canIndicate()) {
              if (pChar->subscribe(true, notifyCB)) {
                Serial.println(">> Subscribed fuer Button-Data!");
              }
            }
          }
        }
      } else {
        Serial.println(">> HID Service nicht gefunden!");
      }
    } else {
      Serial.println(">> Verbindung fehlgeschlagen.");
      NimBLEDevice::getScan()->start(0, false);
    }
  }
  delay(10);
}