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
// Gérard Tyedmers, 2024-01-15 
// Web-Interface added
//
/////////////////////////////////////////////


#include <Arduino.h>
#include <ESP8266WiFi.h>        // v2.4.2
#include <WiFiManager.h>        // v2.0.3-alpha
#include <WiFiUdp.h>
#include <TimeLib.h>            // v1.6.1
#include <Timezone.h>           // v1.2.4
#include <Adafruit_NeoPixel.h>  // v1.10.4
// libraries possibly needed for getUrlParameter
#include <stdio.h>
#include <string.h>

char version[] = "V3";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the web parameters
int rgbRed = 255;
int rgbGreen = 255;
int rgbBlue = 255;
int darkMode = 1;
int rainbow = 0;


// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

int wordClockMinute = 0;
int wordClockHour = 0;
int lastMinuteWordClock = 61;

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
static const char ntpServerName[] = "0.ch.pool.ntp.org";

const int timeZone = 0;     // Central European Time

// function definitions
time_t getNtpTime();

void formatDigits(int digits);

void serialTime();

void sendNTPpacket(IPAddress &address);

//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);
time_t timeWithDST;

WiFiUDP ntpUDP;
unsigned int localPort = 8888;  // local port to listen for UDP packets

static uint32_t Black = Adafruit_NeoPixel::Color(0, 0, 0);
static uint32_t White = Adafruit_NeoPixel::Color(49, 52, 34);
static uint32_t Red = Adafruit_NeoPixel::Color(90, 0, 0);
static uint32_t Green = Adafruit_NeoPixel::Color(10, 90, 0);
static uint32_t Blue = Adafruit_NeoPixel::Color(0, 20, 85);

uint32_t colorDay  = Adafruit_NeoPixel::Color(rgbRed / 5, rgbGreen / 5, rgbBlue / 5);
uint32_t colorNight  = Adafruit_NeoPixel::Color(rgbRed / 25, rgbGreen / 25, rgbBlue / 25);

uint32_t foregroundColor = colorDay;
uint32_t backgroundColor = Black;

// Don't light up WiFi, only on requests
uint32_t wifiColor = Black;

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
static int WordWifi[] = {120, -1};
static int WordNach[] = {42, 41, -1};
static int WordVor[] = {30, 31, 32, -1};

static int SymbolWifi[] = {120, -1};

static int *WordURL[] = {WordFix, WordWifi};

// Stunde
static int WordStundeEins[] = {44, 45, 46, -1};                     // EIS
static int WordStundeZwei[] = {48, 49, 50, 51, -1};                 // ZWÖI
static int WordStundeDrei[] = {52, 53, 54, -1};                     // DRÜ
static int WordStundeVier[] = {64, 63, 62, 61, 60, -1};             // VIERI
static int WordStundeFuenf[] = {59, 58, 57, 56, -1};                // FÜFI
static int WordStundeSechs[] = {66, 67, 68, 69, 70, 71, -1};        // SÄCHSI
static int WordStundeSieben[] = {72, 73, 74, 75, 76, -1};           // SIBNI
static int WordStundeAcht[] = {87, 86, 85, 84, 83, -1};             // ACHTI
static int WordStundeNeun[] = {82, 81, 80, 79, -1};                 // NÜNI
static int WordStundeZehn[] = {89, 90, 91, 92, -1};                 // ZÄNI
static int WordStundeElf[] = {93, 94, 95, 96, -1};                  // EUFI
static int WordStundeZwoelf[] = {106, 105, 104, 103, 102, 101, -1}; // ZWÖUFI

static int *WordStunden[] = {WordStundeZwoelf, WordStundeEins, WordStundeZwei, WordStundeDrei, WordStundeVier,
                             WordStundeFuenf, WordStundeSechs, WordStundeSieben, WordStundeAcht, WordStundeNeun,
                             WordStundeZehn, WordStundeElf, WordStundeZwoelf
};

// Minute
static int WordMinFuenf[] = {8, 9, 10, -1};                   // FÜF
static int WordMinZehn[] = {13, 12, -1};                      // ZÄ
static int WordMinViertel[] = {21, 20, 19, 18, 17, 16, -1};   // VIERTU
static int WordMinZwanzig[] = {22, 23, 24, 25, 26, 27, -1};   // ZWÄNZG
static int WordMinTicks[] = {113, 114, 116, 117, -1};         // ** **

static int *WordMinuten[] = {WordMinFuenf, WordMinZehn, WordMinViertel, WordMinZwanzig, WordMinFuenf};

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(121, D7, NEO_GRB + NEO_KHZ800);

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
void lightup(int *word, uint32_t color) {
  for (int x = 0; x < pixels.numPixels() + 1; x++) {
    if (word[x] == -1) {
      Serial.print(" ");
      break;
    } else {
      pixels.setPixelColor(word[x], color);
    }
  }
}

/**
 * Displays a successful wifi connection
 */
void showWifiSuccess() {
  lightup(WordWifi, Blue);
  lightup(SymbolWifi, Green);
  pixels.show();
}

/**
 * Displays that wifi needs to be configured (select wlan access point wordclock)
 */
void connectWLAN() {
  lightup(WordFix, White);
  lightup(WordWifi, Blue);
  pixels.show();
}

/**
 * Get the value of a URL-Parameter
 */
int extractParameterValue(const char *url, const char *paramName) {
    // Finden der Position von paramName=
    char *paramStart = strstr(url, paramName);
    // Wenn paramName gefunden wurde
    if (paramStart != NULL) {
        // Extrahieren des Substrings, der mit paramName= beginnt
        char *paramSubstring = paramStart + strlen(paramName);
        // Finden der Position des nächsten '&' oder das Ende der Zeichenkette
        char *ampersandPos = strchr(paramSubstring, '&');
        // Wenn '&' gefunden wurde, setzen wir das Ende des Substrings darauf
        if (ampersandPos != NULL) {
            *ampersandPos = '\0';
        }
        // Konvertieren des Substrings in eine Ganzzahl und Rückgabe des Werts
        return atoi(paramSubstring);
    } else {
        return -1; 
    }
}

/**
 * Sets the pixels for the hour
 */
void showHour() {
  if (wordClockMinute < 25) {
    // show this hour if we are before 25 minutes past
    lightup(WordStunden[wordClockHour % 12], foregroundColor);
  } else {
    // show next hour
    lightup(WordStunden[(wordClockHour % 12) + 1], foregroundColor);
  }
}

/**
 * Sets pixels for the current minutes
 */
void showMinute() {
  if (wordClockMinute != 0) {
    if (wordClockMinute >= 5 && wordClockMinute < 30) {
      // sets pixels with minutes array
      lightup(WordMinuten[(wordClockMinute / 5) - 1], foregroundColor);
    } else if (wordClockMinute >= 35) {
      // sets pixels with same array, but in reverse
      lightup(WordMinuten[5 - ((wordClockMinute - 30) / 5)], foregroundColor);
    }
    if ((wordClockMinute >= 5 && wordClockMinute < 25) || (wordClockMinute < 40 && wordClockMinute >= 35)) {
      lightup(WordNach, foregroundColor);
    }
    if (wordClockMinute >= 40 || (wordClockMinute >= 25 && wordClockMinute < 30)) {
      lightup(WordVor, foregroundColor);
    }
    if (wordClockMinute >= 25 && wordClockMinute < 40) {
      lightup(WordHalb, foregroundColor);
    }
    // Checks if the minute ticks should be displayed
    int differenceToLast5Min = wordClockMinute % 5;
    if (differenceToLast5Min != 0) {
      for (int i = 0; i < differenceToLast5Min; i++) {
        pixels.setPixelColor(WordMinTicks[i], foregroundColor);
      }
    }
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

  // Display darker color between 22:00 and 07:00
  if (darkMode == 1 && (wordClockHour >= 22 || wordClockHour < 7)) {
    foregroundColor = colorNight;
  } else {
    foregroundColor = colorDay;
  }

  // light up "it's" it stays on
  lightup(WordEs, foregroundColor);
  lightup(WordIst, foregroundColor);

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
void setWifiStatus(uint32_t color, int duration) {
  wifiWait = duration;
  wifiColor = color;
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
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
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
 * Listen vor changed settings over the web interface and display current time in a loop
 */
void loop() {
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            const char *url = header.c_str();
            if (extractParameterValue(url, "darkmode=") == 1) {
              darkMode = 1;
            } else {
              darkMode = 0;
            }
            if (extractParameterValue(url, "rainbow=") == 1) {
              rainbow = 1;
            } else {
              rainbow = 0;
            }
            if (extractParameterValue(url, "blue=") >= 0) {
              rgbBlue = extractParameterValue(url, "blue=");
            }
            if (extractParameterValue(url, "green=") >= 0) {
              rgbGreen = extractParameterValue(url, "green=");
            }
            if (extractParameterValue(url, "red=") >= 0) {
              rgbRed = extractParameterValue(url, "red=");
            }
            
            colorDay  = Adafruit_NeoPixel::Color(rgbRed / 5, rgbGreen / 5, rgbBlue / 5);
            colorNight  = Adafruit_NeoPixel::Color(rgbRed / 25, rgbGreen / 25, rgbBlue / 25);
            lastMinuteWordClock = 61;
            
            // Display the HTML web page
            // Head 
            client.println("<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\"><title>grrd s WordClock</title><meta name=\"description\" content=\"grrd s WordClock is a web WordClock and a user interface for the Wemos Mini D1 Lite Clock\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><meta name=\"theme-color\" content=\"#444\"><meta name=\"apple-mobile-web-app-title\" content=\"WordClock\"><meta name=\"apple-mobile-web-app-capable\" content=\"yes\"><meta name=\"apple-mobile-web-app-status-bar-style\" content=\"black\">");
            // Styles
            client.println("<style>html {height: 100%;user-select: none;}body {background: linear-gradient(#444, #222);min-width: 100vw;margin: 0;position:fixed;overflow:hidden;font-family: Arial, sans-serif;font-size: large;color: white;text-shadow: 1px 1px 2px #000;height: 100%;}.page {width: 100vw;position: fixed;top: 0;left: 0;right: 0;bottom: 0;background: linear-gradient(#444, #222);}.glow, .M5 .M5, .M10 .M10, .M15 .M15, .M20 .M20, .M30 .M30, .MV .MV, .MA .MA, .H1 .H1, .H2 .H2, .H3 .H3, .H4 .H4, .H5 .H5, .H6 .H6, .H7 .H7, .H8 .H8, .H9 .H9, .H10 .H10, .H11 .H11, .H12 .H12, .M1 .M1, .M2 .M1, .M3 .M1, .M4 .M1, .M2 .M2, .M3 .M2, .M4 .M2, .M3 .M3, .M4 .M3, .M4 .M4 {fill: #fff;text-shadow:0 0 10px #fff;}a:link {color: #ee1149;}a:visited {color: #de0139;}a:hover, a:focus {color: #ff2162;}a:active {color: #ff2162;}#settings, #settingsClose {position: absolute;right: 4vmin;bottom: 4vmin;}.svgButton {width: 4.5vmin;height: 4.5vmin;stroke: #555;stroke-linejoin: round;stroke-width:6;fill:none;}.svgButton:hover {stroke: #fff;text-shadow:0 0 10px #fff;cursor: pointer;}#pageSettings {transform: translateX(100vw);visibility: hidden;opacity: 0;}.pageContent {display: block;left: 0;right: 0;margin: auto;width: 600px;max-width: calc(100vw - 40px);}.pageBody {display: flex;flex-direction: row;justify-content: space-between;margin-bottom: 20px;align-items: center;}.pageFooter {margin-top: 40px;}#color {-webkit-appearance: none;-moz-appearance: none;appearance: none;background-color: transparent;width: 4.5vmin;height: 4.5vmin;border: none;cursor: pointer;}#color::-webkit-color-swatch {border-radius: 50%;border: 0.45vmin solid #555;}#color::-moz-color-swatch {border-radius: 50%;border: 0.45vmin solid #555;}#color::-webkit-color-swatch:hover {border: 0.45vmin solid #fff;}#color::-moz-color-swatch:hover {border: 0.45vmin solid #fff;}a:link {color: #ee1149;}a:visited {filter: brightness(85%);}a:hover, a:focus {filter: brightness(125%);}a:active {filter: brightness(125%);}.hide {display: none;}.dark {filter: brightness(60%);}.swipe-in {animation-name: swipe-in;animation-fill-mode: forwards;animation-duration: 0.7s;}@keyframes swipe-in {0% {transform: translateX(100vw);visibility: hidden;opacity: 0;}1% {transform: translateX(100vw);visibility: visible;opacity: 1;}100% {transform: translateX(0);visibility: visible;opacity: 1;}}.swipe-out {animation-name: swipe-out;animation-fill-mode: forwards;animation-duration: 0.7s;}@keyframes swipe-out {0% {transform: translateX(0);visibility: visible;opacity: 1;}99% {transform: translateX(-100vw);visibility: visible;opacity: 1;}100% {transform: translateX(-100vw);visibility: hidden;opacity: 0;}}.swipe-in-left {animation-name: swipe-in-left;animation-fill-mode: forwards;animation-duration: 0.7s;}@keyframes swipe-in-left {0% {transform: translateX(0);visibility: visible;opacity: 1;}99% {transform: translateX(100vw);visibility: visible;opacity: 1;}100% {transform: translateX(100vw);visibility: hidden;opacity: 0;}}.swipe-out-right {animation-name: swipe-out-right;animation-fill-mode: forwards;animation-duration: 0.7s;}@keyframes swipe-out-right {0% {transform: translateX(-100vw);visibility: hidden;opacity: 0;}1% {transform: translateX(-100vw);visibility: visible;opacity: 1;}100% {transform: translateX(0);visibility: visible;opacity: 1;}}</style>");
            // Body
            client.println("</head><body><div id=\"pageClock\" class=\"page\"><svg id=\"clock\" viewBox=\"0 0 115 110\" preserveAspectRatio=\"xMidYMid slice\" role=\"img\" style=\"font:6px sans-serif;fill:#555;text-shadow:none;text-anchor:middle;width:100vmin;margin:auto;display:block\"><text x=\"07\" y=\"10\" class=\"glow\">E</text><text x=\"17\" y=\"10\" class=\"glow\">S</text><text x=\"27\" y=\"10\">D</text><text x=\"37\" y=\"10\" class=\"glow\">I</text><text x=\"47\" y=\"10\" class=\"glow\">S</text><text x=\"57\" y=\"10\" class=\"glow\">C</text><text x=\"67\" y=\"10\" class=\"glow\">H</text><text x=\"77\" y=\"10\">W</text><text x=\"87\" y=\"10\" class=\"M5\">F</text><text x=\"97\" y=\"10\" class=\"M5\">Ü</text><text x=\"107\" y=\"10\" class=\"M5\">F</text><text x=\"07\" y=\"20\" class=\"M15\">V</text><text x=\"17\" y=\"20\" class=\"M15\">I</text><text x=\"27\" y=\"20\" class=\"M15\">E</text><text x=\"37\" y=\"20\" class=\"M15\">R</text><text x=\"47\" y=\"20\" class=\"M15\">T</text><text x=\"57\" y=\"20\" class=\"M15\">U</text><text x=\"67\" y=\"20\">T</text><text x=\"77\" y=\"20\">Y</text><text x=\"87\" y=\"20\" class=\"M10\">Z</text><text x=\"97\" y=\"20\" class=\"M10\">Ä</text><text x=\"107\" y=\"20\" class=\"M10\">Ä</text><text x=\"07\" y=\"30\" class=\"M20\">Z</text><text x=\"17\" y=\"30\" class=\"M20\">W</text><text x=\"27\" y=\"30\" class=\"M20\">Ä</text><text x=\"37\" y=\"30\" class=\"M20\">N</text><text x=\"47\" y=\"30\" class=\"M20\">Z</text><text x=\"57\" y=\"30\" class=\"M20\">G</text><text x=\"67\" y=\"30\">Q</text><text x=\"77\" y=\"30\">D</text><text x=\"87\" y=\"30\" class=\"MV\">V</text><text x=\"97\" y=\"30\" class=\"MV\">O</text><text x=\"107\" y=\"30\" class=\"MV\">R</text><text x=\"07\" y=\"40\">K</text><text x=\"17\" y=\"40\" class=\"MA\">A</text><text x=\"27\" y=\"40\" class=\"MA\">B</text><text x=\"37\" y=\"40\">D</text><text x=\"47\" y=\"40\" class=\"M30\">H</text><text x=\"57\" y=\"40\" class=\"M30\">A</text><text x=\"67\" y=\"40\" class=\"M30\">U</text><text x=\"77\" y=\"40\" class=\"M30\">B</text><text x=\"87\" y=\"40\" class=\"M30\">I</text><text x=\"97\" y=\"40\">T</text><text x=\"107\" y=\"40\">Z</text><text x=\"07\" y=\"50\" class=\"H1\">E</text><text x=\"17\" y=\"50\" class=\"H1\">I</text><text x=\"27\" y=\"50\" class=\"H1\">S</text><text x=\"37\" y=\"50\">Q</text><text x=\"47\" y=\"50\" class=\"H2\">Z</text><text x=\"57\" y=\"50\" class=\"H2\">W</text><text x=\"67\" y=\"50\" class=\"H2\">Ö</text><text x=\"77\" y=\"50\" class=\"H2\">I</text><text x=\"87\" y=\"50\" class=\"H3\">D</text><text x=\"97\" y=\"50\" class=\"H3\">R</text><text x=\"107\" y=\"50\" class=\"H3\">Ü</text><text x=\"07\" y=\"60\">Z</text><text x=\"17\" y=\"60\" class=\"H4\">V</text><text x=\"27\" y=\"60\" class=\"H4\">I</text><text x=\"37\" y=\"60\" class=\"H4\">E</text><text x=\"47\" y=\"60\" class=\"H4\">R</text><text x=\"57\" y=\"60\" class=\"H4\">I</text><text x=\"67\" y=\"60\" class=\"H5\">F</text><text x=\"77\" y=\"60\" class=\"H5\">Ü</text><text x=\"87\" y=\"60\" class=\"H5\">F</text><text x=\"97\" y=\"60\" class=\"H5\">I</text><text x=\"107\" y=\"60\">T</text><text x=\"07\" y=\"70\" class=\"H6\">S</text><text x=\"17\" y=\"70\" class=\"H6\">Ä</text><text x=\"27\" y=\"70\" class=\"H6\">C</text><text x=\"37\" y=\"70\" class=\"H6\">H</text><text x=\"47\" y=\"70\" class=\"H6\">S</text><text x=\"57\" y=\"70\" class=\"H6\">I</text><text x=\"67\" y=\"70\" class=\"H7\">S</text><text x=\"77\" y=\"70\" class=\"H7\">I</text><text x=\"87\" y=\"70\" class=\"H7\">B</text><text x=\"97\" y=\"70\" class=\"H7\">N</text><text x=\"107\" y=\"70\" class=\"H7\">I</text><text x=\"07\" y=\"80\" class=\"H8\">A</text><text x=\"17\" y=\"80\" class=\"H8\">C</text><text x=\"27\" y=\"80\" class=\"H8\">H</text><text x=\"37\" y=\"80\" class=\"H8\">T</text><text x=\"47\" y=\"80\" class=\"H8\">I</text><text x=\"57\" y=\"80\" class=\"H9\">N</text><text x=\"67\" y=\"80\" class=\"H9\">Ü</text><text x=\"77\" y=\"80\" class=\"H9\">N</text><text x=\"87\" y=\"80\" class=\"H9\">I</text><text x=\"97\" y=\"80\">O</text><text x=\"107\" y=\"80\">F</text><text x=\"07\" y=\"90\">C</text><text x=\"17\" y=\"90\" class=\"H10\">Z</text><text x=\"27\" y=\"90\" class=\"H10\">Ä</text><text x=\"37\" y=\"90\" class=\"H10\">N</text><text x=\"47\" y=\"90\" class=\"H10\">I</text><text x=\"57\" y=\"90\" class=\"H11\">E</text><text x=\"67\" y=\"90\" class=\"H11\">U</text><text x=\"77\" y=\"90\" class=\"H11\">F</text><text x=\"87\" y=\"90\" class=\"H11\">I</text><text x=\"97\" y=\"90\">D</text><text x=\"107\" y=\"90\">I</text><text x=\"07\" y=\"100\">O</text><text x=\"17\" y=\"100\">K</text><text x=\"27\" y=\"100\">G</text><text x=\"37\" y=\"100\" class=\"H12\">Z</text><text x=\"47\" y=\"100\" class=\"H12\">W</text><text x=\"57\" y=\"100\" class=\"H12\">Ö</text><text x=\"67\" y=\"100\" class=\"H12\">U</text><text x=\"77\" y=\"100\" class=\"H12\">F</text><text x=\"87\" y=\"100\" class=\"H12\">I</text><text x=\"97\" y=\"100\">J</text><text x=\"107\" y=\"100\">X</text><text x=\"07\" y=\"110\">L</text><text x=\"17\" y=\"110\">Y</text><text x=\"27\" y=\"110\">B</text><text x=\"37\" y=\"110\" class=\"M1\">*</text><text x=\"47\" y=\"110\" class=\"M2\">*</text><text x=\"57\" y=\"110\">P</text><text x=\"67\" y=\"110\" class=\"M3\">*</text><text x=\"77\" y=\"110\" class=\"M4\">*</text><text x=\"87\" y=\"110\">M</text><text x=\"97\" y=\"110\">K</text><g stroke=\"#555\" fill=\"none\" stroke-width=\"0.7\"><path d=\"M 106 109.8 Q 106 106.4 109.4 106.4\"/><path d=\"M 107.2 109.8 Q 107.2 107.8 109.4 107.8\"/></g><circle cx=\"108.8\" cy=\"109.5\" r=\"0.6\" fill=\"#555\"/></svg><svg id=\"settings\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 74 74\"><path d=\"M30 3 A 37 37 0 0 1 44 3 L 44 13 A 25 25 0 0 1 54.5 20 L 63 14 A 37 37 0 0 1 70 25.5 L 61 31 A 25 25 0 0 1 61 42.5 L 70 48.5 A 37 37 0 0 1 63 60 L 54.5 54 A 25 25 0 0 1 44 61 L 44 71 A 37 37 0 0 1 30 71 L 30 61 A 25 25 0 0 1 19.5 54 L 11 60 A 37 37 0 0 1 4 48.5 L 13 42.5 A 25 25 0 0 1 13 31 L 4 25.5 A 37 37 0 0 1 11 14 L 19.5 20 A 25 25 0 0 1 30 13 Z\"/><circle cx=\"37\" cy=\"37\" r=\"12\"/></svg></div><div id=\"pageSettings\" class=\"page\"><div class=\"pageContent\"><div class=\"pageHead\"><svg id=\"clock\" viewBox=\"5 0 110 30\" preserveAspectRatio=\"xMidYMid slice\" role=\"img\" style=\"font:6px sans-serif;fill:#555;text-shadow:none;text-anchor:middle;width:100%\"><text x=\"07\" y=\"10\">E</text><text x=\"17\" y=\"10\">W</text><text x=\"27\" y=\"10\">F</text><text x=\"37\" y=\"10\" class=\"glow\">G</text><text x=\"47\" y=\"10\" class=\"glow\">R</text><text x=\"57\" y=\"10\" class=\"glow\">R</text><text x=\"67\" y=\"10\" class=\"glow\">D</text><text x=\"77\" y=\"10\">C</text><text x=\"87\" y=\"10\" class=\"glow\">S</text><text x=\"97\" y=\"10\">A</text><text x=\"107\" y=\"10\">J</text><text x=\"07\" y=\"20\">N</text><text x=\"17\" y=\"20\" class=\"glow\">W</text><text x=\"27\" y=\"20\" class=\"glow\">O</text><text x=\"37\" y=\"20\" class=\"glow\">R</text><text x=\"47\" y=\"20\" class=\"glow\">D</text><text x=\"57\" y=\"20\">U</text><text x=\"67\" y=\"20\" class=\"glow\">C</text><text x=\"77\" y=\"20\" class=\"glow\">L</text><text x=\"87\" y=\"20\" class=\"glow\">O</text><text x=\"97\" y=\"20\" class=\"glow\">C</text><text x=\"107\" y=\"20\" class=\"glow\">K</text></svg></div><div class=\"pageBody\"><label for=\"color\">Weli Farb wosch?</label><input type=\"color\" id=\"color\" name=\"head\" value=\"#ffffff\"></div><div class=\"pageBody\"><label for=\"color\">Cha mi nid entscheide. Chli vo auem.</label><svg id=\"rainbowMode\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 70 70\"><path transform=\"scale(0.9) translate(5,5)\" d=\"M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z\"/><path class=\"hide\" transform=\"scale(0.85) translate(5,5)\" d=\"M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z\"/></svg></div><div class=\"pageBody\"><label for=\"color\">Ir Nacht chli weniger häu.</label><svg id=\"darkMode\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 70 70\"><path class=\"hide\" transform=\"scale(0.9) translate(5,5)\" d=\"M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z\"/><path transform=\"scale(0.85) translate(5,5)\" d=\"M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z\"/></svg></div><div class=\"pageFooter\"><p class=\"popup-content\">Handgmachti Software us Bärn</p><p class=\"popup-content\">Gérard Tyedmers -<a href=\"https://grrd.ch\">grrd.ch</a>-<a href=\"mailto:grrd@gmx.net\">grrd@gmx.net</a></p></div><svg id=\"settingsClose\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 70 70\"><path transform=\"scale(0.85) translate(5,5)\" d=\"M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z\"/></svg></div></div>");
            // Script
            client.println("<script>{let a,l,d,o=1,c=0,n=255,i=0,r=0;const g=document.getElementById(\"clock\"),m=document.getElementById(\"pageClock\"),u=document.getElementById(\"pageSettings\"),L=document.getElementById(\"color\");function e(e){document.styleSheets[0].cssRules[3].style.fill=e,document.styleSheets[0].cssRules[3].style.textShadow=\"0 0 10px \"+e,document.getElementsByTagName(\"a\")[0].style.color=e,document.getElementsByTagName(\"a\")[1].style.color=e}function t(t){t!==c&&(document.getElementById(\"rainbowMode\").children[0].classList.toggle(\"hide\"),document.getElementById(\"rainbowMode\").children[1].classList.toggle(\"hide\")),c=t,c||e(L.value)}function s(e){e!==o&&(document.getElementById(\"darkMode\").children[0].classList.toggle(\"hide\"),document.getElementById(\"darkMode\").children[1].classList.toggle(\"hide\")),o=e}setInterval((function(){a=new Date,l=a.getHours()%12,c&&(n&&!r?(n-=1,i+=1):i?(i-=1,r+=1):(r-=1,n+=1),e(\"rgb(\"+n+\", \"+i+\", \"+r+\")\")),o&&(a.getHours()>=22||a.getHours()<7)?document.getElementsByTagName(\"body\")[0].classList.add(\"dark\"):document.getElementsByTagName(\"body\")[0].classList.remove(\"dark\"),d!==a.getMinutes()&&(d=a.getMinutes(),g.classList.remove(...g.classList),d>=55?g.classList.add(\"M5\",\"MV\"):d>=50?g.classList.add(\"M10\",\"MV\"):d>=45?g.classList.add(\"M15\",\"MV\"):d>=40?g.classList.add(\"M20\",\"MV\"):d>=35?g.classList.add(\"M5\",\"MA\",\"M30\"):d>=30?g.classList.add(\"M30\"):d>=25?g.classList.add(\"M5\",\"MV\",\"M30\"):d>=20?g.classList.add(\"M20\",\"MA\"):d>=15?g.classList.add(\"M15\",\"MA\"):d>=10?g.classList.add(\"M10\",\"MA\"):d>=5&&g.classList.add(\"M5\",\"MA\"),d>=25&&(l+=1),g.classList.add(\"H\"+l.toString()),g.classList.add(\"M\"+(d%5).toString()))}),100),document.getElementById(\"settings\").addEventListener(\"click\",(function(){document.activeElement.blur(),m.classList.remove(\"swipe-out-right\"),u.classList.remove(\"swipe-in-left\"),m.classList.add(\"swipe-out\"),u.classList.add(\"swipe-in\")})),document.getElementById(\"settingsClose\").addEventListener(\"click\",(function(){let e=parseInt(L.value.substring(1,3),16),t=parseInt(L.value.substring(3,5),16),s=parseInt(L.value.substring(5,7),16);m.classList.remove(\"swipe-out\"),u.classList.remove(\"swipe-in\"),m.classList.add(\"swipe-out-right\"),u.classList.add(\"swipe-in-left\"),localStorage.setItem(\"wc_color\",L.value),localStorage.setItem(\"wc_rainbow\",c),localStorage.setItem(\"wc_darkmode\",o),setTimeout((function(){window.location.search=\"&red=\"+e+\"&green=\"+t+\"&blue=\"+s+\"&rainbow=\"+c+\"&darkmode=\"+o}),500)})),document.getElementById(\"color\").addEventListener(\"change\",t=>{e(L.value)},!1),document.getElementById(\"rainbowMode\").addEventListener(\"click\",e=>{t(1-c)}),document.getElementById(\"darkMode\").addEventListener(\"click\",e=>{s(1-o)}),localStorage.getItem(\"wc_color\")&&(L.value=localStorage.getItem(\"wc_color\"),e(L.value)),localStorage.getItem(\"wc_rainbow\")&&t(parseInt(localStorage.getItem(\"wc_rainbow\"))),localStorage.getItem(\"wc_darkmode\")&&s(parseInt(localStorage.getItem(\"wc_darkmode\")))}</script></body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }


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
