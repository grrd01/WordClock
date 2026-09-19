/*
 * ==================================================================================
 * ShanWan Q36 HID-Mode Decoder (Für "Q36 for Android" Modus)
 * Controller im D-Modus pairen, danach 1 * Home drücken, bis LED von rot zu gelb wechselt
 * ==================================================================================
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

static BLEUUID hidServiceUUID((uint16_t)0x1812); // Der offizielle HID Service
static BLEUUID reportCharUUID((uint16_t)0x2A4D); // Der Report-Kanal

static boolean doConnect = false;
static boolean connected = false;
static BLEAdvertisedDevice* myDevice = nullptr;

class MySecurityCallbacks : public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() { return 0; }
  void onPassKeyNotify(uint32_t pass_key) {}
  bool onSecurityRequest() { return true; }
  bool onConnectConfirm() { return true; }
};

class MyClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) { Serial.println("-> Physisch verbunden!"); }
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("-> Verbindung verloren.");
  }
};

// Callback für die Tasten im HID-Modus
static void hidNotifyCallback(BLERemoteCharacteristic* pChara, uint8_t* pData, size_t length, bool isNotify) {
    // Wir loggen JEDE Änderung ungefiltert
    Serial.printf("HID-DATA (L=%d): ", length);
    for (size_t i = 0; i < length; i++) {
        Serial.printf("%02X ", pData[i]);
    }
    Serial.println();
    
    // Einfache Live-Analyse für die 2-Byte Tastatur-Codes
    if (length == 2) {
        if (pData[0] == 0xCD) Serial.println("-> [Taste A erkannt!]");
        if (pData[0] == 0x24) Serial.println("-> [Taste B erkannt!]");
        if (pData[0] == 0x23) Serial.println("-> [Taste X erkannt!]");
        if (pData[0] == 0xE2) Serial.println("-> [Taste Y erkannt!]");
        if (pData[0] == 0xE9) Serial.println("-> [Taste Up erkannt!]");
        if (pData[0] == 0xEA) Serial.println("-> [Taste Down erkannt!]");
        if (pData[0] == 0xB6) Serial.println("-> [Taste Left erkannt!]");
        if (pData[0] == 0xB5) Serial.println("-> [Taste Right erkannt!]");
        if (pData[0] == 0xE9) Serial.println("-> [Taste L erkannt!]");
        if (pData[0] == 0xB6) Serial.println("-> [Taste L2 erkannt!]");
        if (pData[0] == 0xEA) Serial.println("-> [Taste R erkannt!]");
        if (pData[0] == 0xB5) Serial.println("-> [Taste R2 erkannt!]");
        if (pData[0] == 0xEA) Serial.println("-> [Taste Select erkannt!]");
        if (pData[0] == 0xE9) Serial.println("-> [Taste Start erkannt!]");
        if (pData[0] == 0x00 && pData[1] == 0x00) Serial.println("-> [Taste losgelassen]");
    }
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) { 
    String name = advertisedDevice.getName().c_str();
    name.toLowerCase();
    
    // Sucht nach dem neuen Namen im HID-Modus
    if (name.indexOf("q36") >= 0 || name.indexOf("android") >= 0) {
      Serial.printf("Gamepad gefunden: %s\n", advertisedDevice.getName().c_str());
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

bool connectToServer() {
    BLEClient* pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallbacks());

    if (!pClient->connect(myDevice)) {
        Serial.println("Verbindung fehlgeschlagen.");
        return false;
    }
    
    pClient->setMTU(40);
    delay(2000); // Wichtig für den Key-Exchange

    Serial.println("Suche gezielt nach HID-Report-Kanälen...");
    auto services = pClient->getServices();
    int activeChannels = 0;

    for (auto servicePair : *services) {
        BLERemoteService* pService = servicePair.second;
        // Wir suchen im HID-Service nach allen Report-Charakteristiken
        if (pService->getUUID().equals(hidServiceUUID) || true) { 
            auto characteristics = pService->getCharacteristics();
            for (auto charPair : *characteristics) {
                BLERemoteCharacteristic* pChara = charPair.second;
                
                if (pChara->getUUID().equals(reportCharUUID) && pChara->canNotify()) {
                    pChara->registerForNotify(hidNotifyCallback);
                    
                    BLERemoteDescriptor* pDec = pChara->getDescriptor(BLEUUID((uint16_t)0x2902));
                    if (pDec != nullptr) {
                        uint8_t val[] = {0x01, 0x00};
                        pDec->writeValue(val, 2, true);
                    }
                    Serial.printf("-> HID-Kanal aktiv (Handle: 0x%02X)\n", pChara->getHandle());
                    activeChannels++;
                }
            }
        }
    }

    if (activeChannels > 0) {
        Serial.println("\n>>> BEREIT! Drücke jetzt nacheinander verschiedene Tasten (A, B, Home, R...)");
        connected = true;
    }
    return connected;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  BLEDevice::init("XIAO-HID-Host");
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
    connectToServer();
    doConnect = false;
  }
  delay(10);
}