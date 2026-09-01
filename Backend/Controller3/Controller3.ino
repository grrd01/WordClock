#include <NimBLEDevice.h>

static const char *TARGET_A = "ShanWan";
static const char *TARGET_B = "Q36";

NimBLEClient *gClient = nullptr;

bool looksLikeQ36(const std::string &name) {
  return (!name.empty() &&
          (name.find(TARGET_A) != std::string::npos ||
           name.find(TARGET_B) != std::string::npos));
}

void notifyCb(NimBLERemoteCharacteristic *c, uint8_t *data, size_t len, bool isNotify) {
  Serial.print(isNotify ? "N " : "I ");
  Serial.print(c->getUUID().toString().c_str());
  Serial.print(" : ");

  for (size_t i = 0; i < len; ++i) {
    if (data[i] < 16) {
      Serial.print('0');
    }
    Serial.print(data[i], HEX);
    if (i + 1 < len) {
      Serial.print(' ');
    }
  }
  Serial.println();
}

bool findQ36Address(NimBLEAddress &addrOut) {
  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(45);
  scan->setWindow(15);

  if (!scan->start(3, false)) {
    return false;
  }

  NimBLEScanResults results = scan->getResults();
  for (int i = 0; i < results.getCount(); ++i) {
    const NimBLEAdvertisedDevice *d = results.getDevice(i);

    if (looksLikeQ36(d->getName())) {
      addrOut = d->getAddress();
      scan->clearResults();
      return true;
    }
  }

  scan->clearResults();
  return false;
}

bool connectAndSubscribe(const NimBLEAddress &addr) {
  if (gClient == nullptr) {
    gClient = NimBLEDevice::createClient();
  }

  Serial.print("Connecting to ");
  Serial.println(addr.toString().c_str());

  if (!gClient->connect(addr)) {
    Serial.println("Connect failed");
    return false;
  }

  NimBLERemoteService *hid = gClient->getService(NimBLEUUID((uint16_t)0x1812));
  if (hid == nullptr) {
    Serial.println("HID service 0x1812 not found");
    gClient->disconnect();
    return false;
  }

  NimBLERemoteCharacteristic *report = hid->getCharacteristic(NimBLEUUID((uint16_t)0x2A4D));
  if (report == nullptr) {
    Serial.println("Report char 0x2A4D not found");
    gClient->disconnect();
    return false;
  }

  bool subscribed = false;
  if (report->canNotify()) {
    subscribed = report->subscribe(true, notifyCb);
  } else if (report->canIndicate()) {
    subscribed = report->subscribe(false, notifyCb);
  }

  if (!subscribed) {
    Serial.println("Subscribe failed");
    gClient->disconnect();
    return false;
  }

  Serial.println("Connected. Waiting for raw HID reports...");
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println("Q36 minimal BLE host (ESP32-C6)");

  NimBLEDevice::init("");
}

void loop() {
  if (gClient != nullptr && gClient->isConnected()) {
    delay(20);
    return;
  }

  NimBLEAddress addr;
  if (!findQ36Address(addr)) {
    Serial.println("Q36 not found, rescanning...");
    delay(500);
    return;
  }

  connectAndSubscribe(addr);
  delay(500);
}

