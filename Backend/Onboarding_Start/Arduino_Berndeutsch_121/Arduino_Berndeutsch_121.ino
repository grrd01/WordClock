/////////////////////////////////////////////
//
// LOLIN (WEMOS) D1 mini Lite (ESP8266) Wordclock Program
// Based on sripts and snippets from:
// Rui Santos http://randomnerdtutorials.com
// neotrace https://www.instructables.com/id/WORK-IN-PROGRESS-Ribba-Word-Clock-With-Wemos-D1-Mi/
// and others
//
// Kurt Meister, 2018-12-24 | Edit: 2023-04-29 
// Thanks to Manuel Meister for refactoring and adding automated summertime conversion.
//
// Todo: Bearbeite alle Todo's in diesem Script, besprich die Lösung mit deinem Team
// Todo: Erstelle eine grafische Übersicht über alle Funktionen (void). Wähle eine geeignete Diagramm-Form.
//       Das Diagramm soll alle Aufrufe von Funktionen darstellen und den Zweck jeder Funktion kurz erläutern.
//       Grundlage jedes Arduino-Programms sind die beiden Funktionen void setup und void loop.
//       Setup wird zur Initialisierung einmalig durchlaufen, loop läuft danach in einer Endlosschlaufe.
//
/////////////////////////////////////////////


#include <Arduino.h>
#include <ESP8266WiFi.h>        // v2.4.2
#include <WiFiManager.h>        // v2.0.3-alpha
#include <WiFiUdp.h>
#include <TimeLib.h>            // v1.6.1
#include <Timezone.h>           // v1.2.4
#include <Adafruit_NeoPixel.h>  // v1.10.4

char version[] = "V2";

int wordClockMinute = 0;
int wordClockHour = 0;
int lastMinuteWordClock = 61;

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
static const char ntpServerName[] = "0.ch.pool.ntp.org";

const int timeZone = 0;     // Central European Time

//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);
time_t timeWithDST;

WiFiUDP ntpUDP;
unsigned int localPort = 8888;  // local port to listen for UDP packets

static uint32_t Black = Adafruit_NeoPixel::Color(0, 0, 0);
static uint32_t White = Adafruit_NeoPixel::Color(49, 52, 34);
static uint32_t Green = Adafruit_NeoPixel::Color(10, 90, 0);
static uint32_t Red = Adafruit_NeoPixel::Color(90, 0, 0);
static uint32_t Blue = Adafruit_NeoPixel::Color(0, 20, 85);

/*
| Ä | S | U | I | S | C | H | J | F | Ü | F |
| V | I | E | R | T | U | X | B | Z | Ä | D |
| Z | W | Ä | N | Z | G | F | R | V | O | R |
| R | A | B | I | H | A | U | B | I | N | S |
| E | I | S | C | Z | W | Ö | I | D | R | Ü |
| V | V | I | E | R | I | F | Ü | F | I | E |
| S | Ä | C | H | S | I | S | I | B | N | I |
| D | A | C | H | T | I | N | Ü | N | I | K |
| N | Z | Ä | N | I | E | U | F | I | O | G |
| S | G | Y | Z | W | Ö | U | F | I | W | N |
| W | X | C | * | * | C | * | * | H | L | G |
 */

static int WordEs[] = {0, 1, -1};
static int WordIst[] = {3, 4, 5, 6, -1};
static int WordHalb[] = {39, 38, 37, 36, 35, -1};

static int WordFix[] = {77, 98, 99, -1};
static int SymbolWifi[] = {120, -1};
static int WordNach[] = {42, 41, -1};
static int WordVor[] = {30, 31, 32, -1};

static int *WordURL[] = {WordFix, SymbolWifi};

// Stunde
static int WordStundeEins[] = {44, 45, 46, -1};                     // EIS
static int WordStundeZwei[] = {48, 49, 50, 51, -1};                 // ZWÖI
// Todo: Für jede Stunde braucht es ein Array, welche das Wort definiert. Ergänze die fehlenden.
static int WordStundeZwoelf[] = {106, 105, 104, 103, 102, 101, -1}; // ZWÖUFI

static int *WordStunden[] = {WordStundeZwoelf, WordStundeEins, WordStundeZwei,
  // Todo: Fehlende Stunden in diesem Array ergänzen
  // Todo: Weshalb werden die Stunden-Worte hier noch als Array zusammengehängt? Wo wird dies verwendet?
  // Todo: Finde heraus: Warum wiederholt sich am Ende des Arrays der Wert 12?
  WordStundeZwoelf
};

// Minute
static int WordMinFuenf[] = {8, 9, 10, -1};                   // FÜF
// Todo: Fehlende Minuten ergänzen (10, 15, 20)
static int WordMinTicks[] = {113, 114, 116, 117, -1};

static int *WordMinuten[] = {WordMinFuenf, 
  // Todo: fehlende Minuten in diesem Array ergänzen
  // Todo: Finde heraus: Warum wiederholt sich am Ende des Arrays der Wert 5?
  WordMinFuenf
};

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(121, D7, NEO_GRB + NEO_KHZ800);
uint32_t foregroundColor = White;
uint32_t backgroundColor = Black;


// Don't light up WiFi, only on requests
uint32_t wifiColor = Black;

// How long this state should be displayed
int wifiWait = 0;

/**
 * Runs through all pixels
 * @param color Adafruit_NeoPixel-Color to display
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
 * Sets all pixels to the background-color
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

/**
 * Sets array of pixels to a specific color
 * @param word array with the id's of the pixels
 * @param color Adafruit_NeoPixel-Color to display those pixels
 */
void lightup(int *Word, uint32_t Color) {
  for (int x = 0; x < pixels.numPixels() + 1; x++) {
    if (Word[x] == -1) {
      Serial.print(" ");
      break;
    } else {
      pixels.setPixelColor(Word[x], Color);
    }
  }
}

/**
 * Displays that wifi needs to be configured (select wlan access point wordclock)
 */
void connectWLAN() {
  lightup(WordFix, White);
  lightup(SymbolWifi, Blue);
  pixels.show();
}

/**
 * Sets the pixels for the hour
 */
void showHour() {
  // Todo: Die folgende Zeile zeigt den aktuellen Stundenwert an. Dies stimmt nicht immer; um 9:50 muss die Stunde nicht 9 sein sondern 10.
  // Ergänze was fehlt. Die Variable wordClockMinute kann hilfreich sein.
  // Finde heraus, was der Ausdruck " % 12" bewirkt. Was würde ohne passieren?
  lightup(WordStunden[wordClockHour % 12], foregroundColor);
}

/**
 * Sets pixels for the current minutes values
 */
void showMinute() {
  if (wordClockMinute != 0) {
    // Todo: Der folgende If zeigt das Minuten-Wort zwischen 05 und 25 korrekt an
    // Ergänze den fehlenden Code für die Minuten 35 - 55 (die Worte vor/nach/halb werden später gesetzt)
    if (wordClockMinute >= 5 && wordClockMinute < 30) {
      // sets pixels with minutes array
      lightup(WordMinuten[(wordClockMinute / 5) - 1], foregroundColor);
    }

    // Todo: Zeige das Wort "vor" oder "nach" korrekt an

    // Todo: Wann braucht es das Wort "halb"?

    // Checks if the minute ticks should be displayed
    // Todo: Die vier Sterne im Array WordMinTicks zeigen die verstrichenen Minuten seit dem letzten 5-Minuten-Interval an
    // Setze mit möglichst wenig Code die korrekte Anzahl Sterne.
    // Falls du die Bedeutung des % - Zeichens in der Funktion showHour verstanden hast, kann das helfen.
  }
}

/**
 * Displays the status of the wifi in the loop
 */
void displayWifiStatus() {
  if (wifiWait > 0) {
    wifiWait--;
  } else if (wifiWait == 0) {
    wifiColor = Black;
    wifiWait = -1;
    lightup(SymbolWifi, wifiColor);
    pixels.show();
  }
}

/**
 * Displays the current time
 */
void displayTime() {
  blank();

  // Todo: Die Worte "Es" "ist" müssen immer leuchten

  showMinute();
  showHour();

  displayWifiStatus();

  pixels.show();
}

/**
 * Initialize the display
 */
void setupDisplay() {
  pixels.begin();
  wipe();
}

/**
 * Displays the status of the wifi
 * @param color Adafruit_NeoPixel-Color to display those pixels
 * @param duration milliseconds to display the status
 */
void setWifiStatus(uint32_t Color, int duration) {
  wifiWait = duration;
  wifiColor = Color;
  lightup(SymbolWifi, wifiColor);
  pixels.show();
}

/**
 * Print time via serial output
 */
void serialTime() {
  // digital clock display of the time
  Serial.println("");
  formatDigits(wordClockHour);
  Serial.print(":");
  formatDigits(wordClockMinute);
  Serial.print(" - ");
}

/**
 * utility for digital clock display: prints preceding colon and leading 0
 * @param digits value to format
 */
void formatDigits(int digits) {
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/**
 * Updates the global time values with the time from the NTPClient
 */
void getLocalTime() {
  TimeChangeRule *tcr;
  time_t utc;
  utc = now();
  timeWithDST = CE.toLocal(utc, &tcr);

  // global time values
  wordClockMinute = minute(timeWithDST);
  wordClockHour = hour(timeWithDST);
}

/**
 * Get current time value from a ntp server
 */
time_t getNtpTime() {
  setWifiStatus(Blue, 250);
  IPAddress ntpServerIP; // NTP server's ip address
  while (ntpUDP.parsePacket() > 0); // discard any previously received packets
  Serial.println("");
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 2500) {
    int size = ntpUDP.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      ntpUDP.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long) packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long) packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long) packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long) packetBuffer[43];
      setSyncInterval(120);
      Serial.print("Received: ");
      Serial.println(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
      setWifiStatus(Green, 250);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  setWifiStatus(Red, 500000000);
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time (Timelib just queries again)
}

/**
 * Send an NTP request to the time server at the given address
 */
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 0x49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 0x49;
  packetBuffer[15] = 0x52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  ntpUDP.beginPacket(address, 123); //NTP requests are to port 123
  ntpUDP.write(packetBuffer, NTP_PACKET_SIZE);
  ntpUDP.endPacket();
}

/**
 * Initializes timeClient so it queries the NTP server
 * also makes first update to sync the time
 */
void setupTime() {
  Serial.println("Starting UDP");
  ntpUDP.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(ntpUDP.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(10);
}

/**
 * Sets up wifi
 */
void setupWifi() {
  wifi_station_set_hostname("wordclock");

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();

  // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // Displays Wifi Connect screen
  connectWLAN();

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "WordClock"
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("wordclock");
  // or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
}

/**
 * Main setup to start WordClock
 */
void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Version: ");
  Serial.println(version);
  setupDisplay();

  chase(Green); // run basic screen test and show success

  setupWifi();
  setupTime();
}

/**
 * Displays current time if minute changed
 */
void loop() {
  if (timeStatus() != timeNotSet) {
    if (lastMinuteWordClock != wordClockMinute) { //update the display only if time has changed
      getLocalTime();
      serialTime();
      displayTime();
      lastMinuteWordClock = wordClockMinute;
    } else {
      displayWifiStatus();
      getLocalTime();
    }
  }
}
