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
// Web-Interface added (http://wordclock.local/)
//
/////////////////////////////////////////////


#include <Arduino.h>
#include <ESP8266WiFi.h>        // v2.4.2
#include <ESP8266mDNS.h>
#include <WiFiManager.h>        // v2.0.3-alpha
#include <WiFiUdp.h>
#include <TimeLib.h>            // v1.6.1
#include <Timezone.h>           // v1.2.4
#include <Adafruit_NeoPixel.h>  // v1.10.4

// set name for access-point and mdns-server
const char* version = "wordclock";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the web parameters
int rgbRed = 255;
int rgbGreen = 255;
int rgbBlue = 255;
int power = 1;
int darkMode = 1;
int rainbow = 0;
int rainbowSpeed = 200;
int rainbowWait = 200;


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

//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);
time_t timeWithDST;

WiFiUDP ntpUDP;
unsigned int localPort = 8888;  // local port to listen for UDP packets

static uint32_t Black = Adafruit_NeoPixel::Color(0, 0, 0);
static uint32_t White = Adafruit_NeoPixel::Color(49, 52, 34);
static uint32_t Grey = Adafruit_NeoPixel::Color(3, 3, 3);
static uint32_t Red = Adafruit_NeoPixel::Color(90, 0, 0);
static uint32_t Green = Adafruit_NeoPixel::Color(10, 90, 0);
static uint32_t Blue = Adafruit_NeoPixel::Color(0, 20, 85);
static uint32_t Cornflower = Adafruit_NeoPixel::Color(8, 7, 16);

static uint32_t MastermindColor1 = Adafruit_NeoPixel::Color(100, 0, 5);
static uint32_t MastermindColor2 = Adafruit_NeoPixel::Color(100, 25, 0);
static uint32_t MastermindColor3 = Adafruit_NeoPixel::Color(100, 90, 0);
static uint32_t MastermindColor4 = Adafruit_NeoPixel::Color(10, 95, 0);
static uint32_t MastermindColor5 = Adafruit_NeoPixel::Color(0, 40, 100);
static uint32_t MastermindColor6 = Adafruit_NeoPixel::Color(85, 0, 100);

static uint32_t MastermindColors[] = {MastermindColor1, MastermindColor2, MastermindColor3, MastermindColor4, MastermindColor5, MastermindColor6};

uint32_t colorDay  = Adafruit_NeoPixel::Color(rgbRed / 5, rgbGreen / 5, rgbBlue / 5);
uint32_t colorNight  = Adafruit_NeoPixel::Color(rgbRed / 25, rgbGreen / 25, rgbBlue / 25);

uint32_t foregroundColor = colorDay;
uint32_t backgroundColor = Black;

// Don't light up WiFi, only on requests
uint32_t wifiColor = Black;

/*
| E | S | D | I | S | C | H | W | F | Ü | F |
| V | I | E | R | T | U | T | Z | Ä | Ä | Y |
| Z | W | Ä | N | Z | G | Q | D | V | O | R |
| K | A | B | D | H | A | U | B | I | T | Z |
| E | I | S | Q | Z | W | Ö | I | D | R | Ü |
| Z | V | I | E | R | I | F | Ü | F | I | T |
| G | M | S | Ä | C | H | S | I | B | N | I |
| A | C | H | T | I | N | Ü | N | I | O | F |
| C | D | Z | Ä | N | I | X | E | U | F | I |
| O | K | G | Z | W | Ö | U | F | I | L | X |
| L | Y | B | ° | ° | P | ° | ° | M | K | @ |
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
// FabLab: static int WordStundeSechs[] = {66, 67, 68, 69, 70, 71, -1};        // SÄCHSI
static int WordStundeSechs[] = {68, 69, 70, 71, 72, 73, -1};        // SÄCHSI
static int WordStundeSieben[] = {72, 73, 74, 75, 76, -1};           // SIBNI
static int WordStundeAcht[] = {87, 86, 85, 84, 83, -1};             // ACHTI
static int WordStundeNeun[] = {82, 81, 80, 79, -1};                 // NÜNI
// FabLab: static int WordStundeZehn[] = {89, 90, 91, 92, -1};                 // ZÄNI
static int WordStundeZehn[] = {90, 91, 92, 93, -1};                 // ZÄNI
// FabLab: static int WordStundeElf[] = {93, 94, 95, 96, -1};                  // EUFI
static int WordStundeElf[] = {95, 96, 97, 98, -1};                  // EUFI
static int WordStundeZwoelf[] = {106, 105, 104, 103, 102, 101, -1}; // ZWÖUFI

static int *WordStunden[] = {WordStundeZwoelf, WordStundeEins, WordStundeZwei, WordStundeDrei, WordStundeVier,
                             WordStundeFuenf, WordStundeSechs, WordStundeSieben, WordStundeAcht, WordStundeNeun,
                             WordStundeZehn, WordStundeElf, WordStundeZwoelf
};

// Minute
static int WordMinFuenf[] = {8, 9, 10, -1};                   // FÜF
// FabLab: static int WordMinZehn[] = {13, 12, -1};                      // ZÄ
static int WordMinZehn[] = {14, 13, 12, -1};                      // ZÄ
static int WordMinViertel[] = {21, 20, 19, 18, 17, 16, -1};   // VIERTU
static int WordMinZwanzig[] = {22, 23, 24, 25, 26, 27, -1};   // ZWÄNZG
static int WordMinTicks[] = {113, 114, 116, 117, -1};         // ** **

static int *WordMinuten[] = {WordMinFuenf, WordMinZehn, WordMinViertel, WordMinZwanzig, WordMinFuenf};

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(121, D7, NEO_GRB + NEO_KHZ800);

// How long this state should be displayed
int wifiWait = 0;

// Snake variables
int snake[120];
int snakeLen = 3;
int snakeNext = -1;
int snakeSnack = -2;  // pixel 0-120
int snakeDir = 0; // 1=up, 2=right, 3=down, 4=left, 5=new game, 6=exit game
int snakePrevDir = 0;
int snakeSpeed = 35000;
int snakeWait = 35000;
bool inSnake = false;

// Mastermind variables
int mastermindCode[4];
int mastermindCodeBackup[4];
int mastermindCodeTry[4];
int mastermindTry = 0;
int mastermindPlace = 0;
int mastermindColor = 0;
bool inMastermind = false;

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
 * Moves a pixel n rows down
 */
int down(int pixel, int rows) {
  for (int i = 0; i < rows; i++) {
    pixel = pixel + 1 + 2 * (10 - pixel % 11);
  }
  return pixel;
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
    for (int i = 0; i < differenceToLast5Min; i++) {
      pixels.setPixelColor(WordMinTicks[i], foregroundColor);
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
  wifi_station_set_hostname(version);

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
  wifiManager.autoConnect(version);
  // or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  // Display local ip address on clockface by looping through every character.
  String localIP = WiFi.localIP().toString();
  for (int i = 0; i < localIP.length(); i++) {
    Serial.println(localIP[i]);
    blank();
    if (localIP[i] == '0') {
      pixels.setPixelColor(109, foregroundColor);
    } else if (localIP[i] == '.') {
      pixels.setPixelColor(WordMinTicks[0], foregroundColor);
    } else {
      lightup(WordStunden[localIP[i] - '0'], foregroundColor);
    }
    pixels.show();
    delay(1500);
    blank();
    pixels.show();
    delay(500);
  }

  if (MDNS.begin(version)) {
    Serial.println("DNS gestartet, erreichbar unter: ");
    Serial.println("http://" + String(version) + ".local/");
  }

  server.begin();
  chase(Green);
}

/*
 * places a snack on an empty space in snake game
 */
void setSnack() {
  snakeSnack = -1;
  while (snakeSnack < 0) {
    snakeSnack = random(121);
    for (int i = snakeLen - 1; i >= 0; i--) {
      if (snakeSnack == snake[i]){
        // place occupied by snake
        snakeSnack = -1;
      }
    }
    if (snakeSnack == snakeNext) {
      snakeSnack = -1;
    }
  }
  pixels.setPixelColor(snakeSnack, Red);
}

/*
 * prepares a new mastermind game
 */
void clearMastermind() {
  wipe();
  inMastermind = true;
  randomSeed(micros());
  mastermindCode[0] = random(1,7);
  mastermindCode[1] = random(1,7);
  mastermindCode[2] = random(1,7);
  mastermindCode[3] = random(1,7);
  mastermindTry = 0;
  for (int i = 0; i < 11; i++) {
    pixels.setPixelColor(down(0, i), Grey);
    pixels.setPixelColor(down(5, i), Grey);
    pixels.setPixelColor(down(10, i), Grey);
    pixels.show();
  }
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
  randomSeed(analogRead(A0));
}

/**
 * Listen for requests over the web interface and display current time in a loop
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
            if (header.indexOf("snake") >= 0) {
              // Client is playing snake game:
              const char *url = header.c_str();
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/plain");
              client.println("Access-Control-Allow-Origin: *");
              client.println("Connection: close");
              client.println();
              client.println(snakeLen);
              if (extractParameterValue(url, "dir=") > 0 && extractParameterValue(url, "dir=") < 7) {
                snakePrevDir = snakeDir;
                snakeDir = extractParameterValue(url, "dir=");
              }
              if (!inSnake && snakeDir == 5 && power == 1) {
                // start new snake game
                inSnake = true;
                snake[0] = 49;
                snake[1] = 60;
                snake[2] = 71;
                snake[3] = -1;
                snakeLen = 3;
                snakeDir = 0;
                snakeNext = -1;
                snakeSpeed = 35000;
                blank();
                lightup(snake, Green);
                setSnack();
                pixels.show();
              } else if (inSnake && snakeDir == 6) {
                // exit current snake game
                inSnake = false;
                lastMinuteWordClock = 61;
              }
            } else if (header.indexOf("mastermind") >= 0) {
              // Client is playing mastermind game:
              const char *url = header.c_str();

              mastermindCodeTry[3] = extractParameterValue(url, "c4=");
              if (!inMastermind && mastermindCodeTry[3] == 0 && power == 1) {
                // start new mastermind game
                clearMastermind();
              } else if (inMastermind && mastermindCodeTry[3] == 7) {
                // exit current mastermind game
                inMastermind = false;
                lastMinuteWordClock = 61;
              } else {
                // restart a new game if needed
                if (mastermindTry == 11 || mastermindPlace == 4) {
                  clearMastermind();
                }
                // evaluate players try
                mastermindCodeTry[2] = extractParameterValue(url, "c3=");
                mastermindCodeTry[1] = extractParameterValue(url, "c2=");
                mastermindCodeTry[0] = extractParameterValue(url, "c1=");
                mastermindPlace = 0;
                mastermindColor = 0;
                for (int i = 0; i < 4; i++) {
                  pixels.setPixelColor(down(i + 1, mastermindTry), MastermindColors[mastermindCodeTry[i] - 1]);
                  mastermindCodeBackup[i] = mastermindCode[i];
                  // check right position
                  Serial.println("CheckPlace");
                  Serial.println(mastermindCodeBackup[i]);
                  Serial.println(mastermindCodeTry[i]);
                  if (mastermindCodeTry[i] == mastermindCodeBackup[i]) {
                    mastermindCodeTry[i] = -1;
                    mastermindCodeBackup[i] = -2;
                    mastermindPlace ++;
                    pixels.setPixelColor(down(mastermindPlace + 5, mastermindTry), White);
                  }
                }
                for (int i = 0; i < 4; i++) {
                  for (int j = 0; j < 4; j++) {
                    // check right color
                    if (mastermindCodeTry[i] == mastermindCodeBackup[j]) {
                      mastermindCodeTry[i] = -1;
                      mastermindCodeBackup[j] = -2;
                      mastermindColor ++;
                      pixels.setPixelColor(down(mastermindColor + mastermindPlace + 5, mastermindTry), Cornflower);
                    }
                  }
                }
                pixels.show();
                mastermindTry ++;
              }
              if (mastermindPlace == 4) {
                // player won
                for (int i = 0; i < mastermindTry; i++) {
                  pixels.setPixelColor(down(0, i), Green);
                  pixels.setPixelColor(down(5, i), Green);
                  pixels.setPixelColor(down(10, i), Green);
                  pixels.show();
                }
              } else if (mastermindTry == 11) {
                // player lost
                for (int i = 0; i < mastermindTry; i++) {
                  pixels.setPixelColor(down(0, i), Red);
                  pixels.setPixelColor(down(5, i), Red);
                  pixels.setPixelColor(down(10, i), Red);
                  pixels.show();
                }
              }
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:application/json");
              client.println("Connection: close");
              client.println();
              client.println("{\"place\":");
              client.println(mastermindPlace);
              client.println(", \"color\":");
              client.println(mastermindColor);
              client.println(", \"try\":");
              client.println(mastermindTry);
              client.println("}");

            } else if (header.indexOf("update_params") >= 0) {
              // Get new params from client:
              const char *url = header.c_str();
              if (extractParameterValue(url, "power=") == 1) {
                power = 1;
              } else if (extractParameterValue(url, "power=") == 0) {
                power = 0;
                blank();
                pixels.show();
              }
              if (extractParameterValue(url, "speed=") >= 50 && extractParameterValue(url, "speed=") <= 2000) {
                rainbowSpeed = (extractParameterValue(url, "speed=") - 48) * 4;
              }
              if (extractParameterValue(url, "darkmode=") == 1) {
                darkMode = 1;
              } else if (extractParameterValue(url, "darkmode=") == 0) {
                darkMode = 0;
              }
              if (extractParameterValue(url, "rainbow=") == 1) {
                rainbow = 1;
              } else if (extractParameterValue(url, "rainbow=") == 0) {
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
              if (rainbow == 1) {
                rgbRed = 255;
                rgbGreen = 0;
                rgbBlue = 0;
                rainbowWait = rainbowSpeed;
              }

              colorDay  = Adafruit_NeoPixel::Color(rgbRed / 5, rgbGreen / 5, rgbBlue / 5);
              colorNight  = Adafruit_NeoPixel::Color(rgbRed / 25, rgbGreen / 25, rgbBlue / 25);
              lastMinuteWordClock = 61;
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/plain");
              client.println("Connection: close");
              client.println();
              client.println("OK");
            } else if (header.indexOf("get_params") >= 0) {
              // Send current params to client:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:application/json");
              client.println("Connection: close");
              client.println();
              client.println("{\"red\":");
              client.println(rgbRed);
              client.println(", \"green\":");
              client.println(rgbGreen);
              client.println(", \"blue\":");
              client.println(rgbBlue);
              client.println(", \"rainbow\":");
              client.println(rainbow);
              client.println(", \"darkmode\":");
              client.println(darkMode);
              client.println(", \"speed\":");
              client.println(rainbowSpeed / 4 + 48);
              client.println(", \"power\":");
              client.println(power);
              client.println("}");
            } else {
              // New connection, send web interface to client:
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              // Display the HTML web page
              // Head
              client.println("<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\"><title>grrd s WordClock</title><link id=\"icon\" rel=\"icon\" href=\"data:image/png;base64,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\"><meta name=\"description\" content=\"grrd s WordClock is a web WordClock and a user interface for the Wemos Mini D1 Lite Clock\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><meta name=\"theme-color\" content=\"#444\"><meta name=\"apple-mobile-web-app-title\" content=\"WordClock\"><link id=\"iphone\" rel=\"apple-touch-icon\"><meta name=\"apple-mobile-web-app-capable\" content=\"yes\"><meta name=\"apple-mobile-web-app-status-bar-style\" content=\"black\">");
              // Styles
              client.println("<style>body,html{height:100%}.page,body{position:fixed;background:linear-gradient(#444,#222)}a:link,a:visited{color:var(--main-color)}.page,.pageContent{top:0;left:0;right:0}.swipe-in,.swipe-in-left,.swipe-out,.swipe-out-right{animation-fill-mode:forwards;animation-duration:.7s}:root{--main-color:#878ade}html{user-select:none}body{min-width:100vw;margin:0;overflow:hidden;font-family:Arial,sans-serif;font-size:large;color:#fff;text-shadow:1px 1px 2px #000}.page{width:100vw;bottom:0}#clock,#control,.title{font:6px sans-serif;fill:#555;text-shadow:none;text-anchor:middle;width:100vmin;margin:auto;display:block}@media (orientation:landscape){#control{width:100%;max-width:60vh}}@media (orientation:portrait){#control{width:100%}}.title,.w100{width:100%}#color,.snakeButton,.svgButton,.svgMsg{width:4.5vmin;height:4.5vmin;min-width:30px;min-height:30px}.H0 .H0,.H1 .H1,.H10 .H10,.H11 .H11,.H2 .H2,.H3 .H3,.H4 .H4,.H5 .H5,.H6 .H6,.H7 .H7,.H8 .H8,.H9 .H9,.M1 .M1,.M10 .M10,.M15 .M15,.M2 .M1,.M2 .M2,.M20 .M20,.M3 .M1,.M3 .M2,.M3 .M3,.M30 .M30,.M4 .M1,.M4 .M2,.M4 .M3,.M4 .M4,.M5 .M5,.MA .MA,.MV .MV,.g{fill:var(--main-color);text-shadow:0 0 10px var(--main-color)}.off .g{fill:#555;text-shadow:none}.dark .H0 .H0,.dark .H1 .H1,.dark .H10 .H10,.dark .H11 .H11,.dark .H2 .H2,.dark .H3 .H3,.dark .H4 .H4,.dark .H5 .H5,.dark .H6 .H6,.dark .H7 .H7,.dark .H8 .H8,.dark .H9 .H9,.dark .M1 .M1,.dark .M10 .M10,.dark .M15 .M15,.dark .M2 .M1,.dark .M2 .M2,.dark .M20 .M20,.dark .M3 .M1,.dark .M3 .M2,.dark .M3 .M3,.dark .M30 .M30,.dark .M4 .M1,.dark .M4 .M2,.dark .M4 .M3,.dark .M4 .M4,.dark .M5 .M5,.dark .MA .MA,.dark .MV .MV,.dark .g{filter:brightness(70%)}a:visited{filter:brightness(85%)}a:active,a:focus,a:hover{color:var(--main-color);filter:brightness(125%)}#exitMastermind,#exitSnake,#settings,#settingsClose{position:absolute;right:4vmin;bottom:4vmin}#power{position:absolute;left:4vmin;bottom:4vmin}#scoreMastermind,#scoreSnake{position:absolute;left:4vmin;bottom:4vmin;display:flex;align-items:center}.snakeButton,.svgButton,.svgMsg{stroke:#555;stroke-linejoin:round;stroke-linecap:round;stroke-width:6;fill:none;z-index:1000}circle{pointer-events:none}.snakeButton.g,.snakeButton:hover,.svgButton.g,.svgButton:hover{stroke:#fff;text-shadow:0 0 10px #fff;cursor:pointer}.snakeButton{stroke-width:1.4;fill:#333;fill-opacity:0.01}#pageMastermind,#pageSettings,#pageSnake{transform:translateX(100vw);visibility:hidden;opacity:0}.pageContent{display:block;position:absolute;overflow:auto;margin:0 auto;width:600px;max-width:calc(100vw - 40px);height:100%}.pageBody,.popup-content,.popup-content>span{display:flex;flex-direction:row;justify-content:space-between;margin-bottom:20px;align-items:center;flex-wrap:wrap}.pageFooter{margin-top:60px}#color{-webkit-appearance:none;-moz-appearance:none;appearance:none;background-color:transparent;border:none;cursor:pointer}#color::-webkit-color-swatch{border-radius:50%;border:.45vmin solid #555}#color::-moz-color-swatch{border-radius:50%;border:.45vmin solid #555}#color::-webkit-color-swatch:hover{border:.45vmin solid #fff}#color::-moz-color-swatch:hover{border:.45vmin solid #fff}.hide{display:none}[data-num=\"1\"]{fill:#fc034e}[data-num=\"2\"]{fill:#fc6f03}[data-num=\"3\"]{fill:#fcce03}[data-num=\"4\"]{fill:#18fc03}[data-num=\"5\"]{fill:#0384fc}[data-num=\"6\"]{fill:#f803fc}.swipe-in{animation-name:swipe-in}@keyframes swipe-in{0%{transform:translateX(100vw);visibility:hidden;opacity:0}1%{transform:translateX(100vw);visibility:visible;opacity:1}100%{transform:translateX(0);visibility:visible;opacity:1}}.swipe-out{animation-name:swipe-out}@keyframes swipe-out{0%{transform:translateX(0);visibility:visible;opacity:1}99%{transform:translateX(-100vw);visibility:visible;opacity:1}100%{transform:translateX(-100vw);visibility:hidden;opacity:0}}.swipe-in-left{animation-name:swipe-in-left}@keyframes swipe-in-left{0%{transform:translateX(0);visibility:visible;opacity:1}99%{transform:translateX(100vw);visibility:visible;opacity:1}100%{transform:translateX(100vw);visibility:hidden;opacity:0}}.swipe-out-right{animation-name:swipe-out-right}@keyframes swipe-out-right{0%{transform:translateX(-100vw);visibility:hidden;opacity:0}1%{transform:translateX(-100vw);visibility:visible;opacity:1}100%{transform:translateX(0);visibility:visible;opacity:1}}.slider{-webkit-appearance:none;width:100%;height:4px;border-radius:2px;background:0 0;margin:10px 0;direction:rtl;border:calc(2px + .2vmin) solid #555}.slider::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:3vmin;height:3vmin;min-width:20px;min-height:20px;border-radius:50%;background:var(--main-color);cursor:pointer;outline:#555 solid .45vmin}.slider::-webkit-slider-thumb:hover{outline:#fff solid .45vmin}.slider::-moz-range-thumb{width:3vmin;height:3vmin;min-width:20px;min-height:20px;border-radius:50%;background:var(--main-color);cursor:pointer;outline:#555 solid .45vmin}.slider::-moz-range-thumb:hover{outline:#fff solid .45vmin}</style>");
              // Body
              client.println("</head><body><div id=\"pageClock\" class=\"page\"><svg id=\"clock\" viewBox=\"0 0 115 110\" preserveAspectRatio=\"xMidYMid slice\" role=\"img\"><text x=\"07\" y=\"10\" class=\"g\">E</text><text x=\"17\" y=\"10\" class=\"g\">S</text><text x=\"27\" y=\"10\">D</text><text x=\"37\" y=\"10\" class=\"g\">I</text><text x=\"47\" y=\"10\" class=\"g\">S</text><text x=\"57\" y=\"10\" class=\"g\">C</text><text x=\"67\" y=\"10\" class=\"g\">H</text><text x=\"77\" y=\"10\">W</text><text x=\"87\" y=\"10\" class=\"M5\">F</text><text x=\"97\" y=\"10\" class=\"M5\">Ü</text><text x=\"107\" y=\"10\" class=\"M5\">F</text><text x=\"07\" y=\"20\" class=\"M15\">V</text><text x=\"17\" y=\"20\" class=\"M15\">I</text><text x=\"27\" y=\"20\" class=\"M15\">E</text><text x=\"37\" y=\"20\" class=\"M15\">R</text><text x=\"47\" y=\"20\" class=\"M15\">T</text><text x=\"57\" y=\"20\" class=\"M15\">U</text><text x=\"67\" y=\"20\">T</text><text x=\"77\" y=\"20\" class=\"M10\">Z</text><text x=\"87\" y=\"20\" class=\"M10\">Ä</text><text x=\"97\" y=\"20\" class=\"M10\">Ä</text><text x=\"107\" y=\"20\">Y</text><text x=\"07\" y=\"30\" class=\"M20\">Z</text><text x=\"17\" y=\"30\" class=\"M20\">W</text><text x=\"27\" y=\"30\" class=\"M20\">Ä</text><text x=\"37\" y=\"30\" class=\"M20\">N</text><text x=\"47\" y=\"30\" class=\"M20\">Z</text><text x=\"57\" y=\"30\" class=\"M20\">G</text><text x=\"67\" y=\"30\">Q</text><text x=\"77\" y=\"30\">D</text><text x=\"87\" y=\"30\" class=\"MV\">V</text><text x=\"97\" y=\"30\" class=\"MV\">O</text><text x=\"107\" y=\"30\" class=\"MV\">R</text><text x=\"07\" y=\"40\">K</text><text x=\"17\" y=\"40\" class=\"MA\">A</text><text x=\"27\" y=\"40\" class=\"MA\">B</text><text x=\"37\" y=\"40\">D</text><text x=\"47\" y=\"40\" class=\"M30\">H</text><text x=\"57\" y=\"40\" class=\"M30\">A</text><text x=\"67\" y=\"40\" class=\"M30\">U</text><text x=\"77\" y=\"40\" class=\"M30\">B</text><text x=\"87\" y=\"40\" class=\"M30\">I</text><text x=\"97\" y=\"40\">T</text><text x=\"107\" y=\"40\">Z</text><text x=\"07\" y=\"50\" class=\"H1\">E</text><text x=\"17\" y=\"50\" class=\"H1\">I</text><text x=\"27\" y=\"50\" class=\"H1\">S</text><text x=\"37\" y=\"50\">Q</text><text x=\"47\" y=\"50\" class=\"H2\">Z</text><text x=\"57\" y=\"50\" class=\"H2\">W</text><text x=\"67\" y=\"50\" class=\"H2\">Ö</text><text x=\"77\" y=\"50\" class=\"H2\">I</text><text x=\"87\" y=\"50\" class=\"H3\">D</text><text x=\"97\" y=\"50\" class=\"H3\">R</text><text x=\"107\" y=\"50\" class=\"H3\">Ü</text><text x=\"07\" y=\"60\">Z</text><text x=\"17\" y=\"60\" class=\"H4\">V</text><text x=\"27\" y=\"60\" class=\"H4\">I</text><text x=\"37\" y=\"60\" class=\"H4\">E</text><text x=\"47\" y=\"60\" class=\"H4\">R</text><text x=\"57\" y=\"60\" class=\"H4\">I</text><text x=\"67\" y=\"60\" class=\"H5\">F</text><text x=\"77\" y=\"60\" class=\"H5\">Ü</text><text x=\"87\" y=\"60\" class=\"H5\">F</text><text x=\"97\" y=\"60\" class=\"H5\">I</text><text x=\"107\" y=\"60\">T</text><text x=\"07\" y=\"70\">G</text><text x=\"17\" y=\"70\">M</text><text x=\"27\" y=\"70\" class=\"H6\">S</text><text x=\"37\" y=\"70\" class=\"H6\">Ä</text><text x=\"47\" y=\"70\" class=\"H6\">C</text><text x=\"57\" y=\"70\" class=\"H6\">H</text><text x=\"67\" y=\"70\" class=\"H6 H7\">S</text><text x=\"77\" y=\"70\" class=\"H6 H7\">I</text><text x=\"87\" y=\"70\" class=\"H7\">B</text><text x=\"97\" y=\"70\" class=\"H7\">N</text><text x=\"107\" y=\"70\" class=\"H7\">I</text><text x=\"07\" y=\"80\" class=\"H8\">A</text><text x=\"17\" y=\"80\" class=\"H8\">C</text><text x=\"27\" y=\"80\" class=\"H8\">H</text><text x=\"37\" y=\"80\" class=\"H8\">T</text><text x=\"47\" y=\"80\" class=\"H8\">I</text><text x=\"57\" y=\"80\" class=\"H9\">N</text><text x=\"67\" y=\"80\" class=\"H9\">Ü</text><text x=\"77\" y=\"80\" class=\"H9\">N</text><text x=\"87\" y=\"80\" class=\"H9\">I</text><text x=\"97\" y=\"80\">O</text><text x=\"107\" y=\"80\">F</text><text x=\"07\" y=\"90\">C</text><text x=\"17\" y=\"90\">D</text><text x=\"27\" y=\"90\" class=\"H10\">Z</text><text x=\"37\" y=\"90\" class=\"H10\">Ä</text><text x=\"47\" y=\"90\" class=\"H10\">N</text><text x=\"57\" y=\"90\" class=\"H10\">I</text><text x=\"67\" y=\"90\">X</text><text x=\"77\" y=\"90\" class=\"H11\">E</text><text x=\"87\" y=\"90\" class=\"H11\">U</text><text x=\"97\" y=\"90\" class=\"H11\">F</text><text x=\"107\" y=\"90\" class=\"H11\">I</text><text x=\"07\" y=\"100\">O</text><text x=\"17\" y=\"100\">K</text><text x=\"27\" y=\"100\">G</text><text x=\"37\" y=\"100\" class=\"H0\">Z</text><text x=\"47\" y=\"100\" class=\"H0\">W</text><text x=\"57\" y=\"100\" class=\"H0\">Ö</text><text x=\"67\" y=\"100\" class=\"H0\">U</text><text x=\"77\" y=\"100\" class=\"H0\">F</text><text x=\"87\" y=\"100\" class=\"H0\">I</text><text x=\"97\" y=\"100\">L</text><text x=\"107\" y=\"100\">X</text><text x=\"07\" y=\"110\">L</text><text x=\"17\" y=\"110\">Y</text><text x=\"27\" y=\"110\">B</text><text x=\"37\" y=\"112.5\" class=\"M1\" font-size=\"8\">°</text><text x=\"47\" y=\"112.5\" class=\"M2\" font-size=\"8\">°</text><text x=\"57\" y=\"110\">P</text><text x=\"67\" y=\"112.5\" class=\"M3\" font-size=\"8\">°</text><text x=\"77\" y=\"112.5\" class=\"M4\" font-size=\"8\">°</text><text x=\"87\" y=\"110\">M</text><text x=\"97\" y=\"110\">K</text><g stroke=\"#555\" fill=\"none\" stroke-width=\"0.7\"><path d=\"M 106 109.8 Q 106 106.4 109.4 106.4\"/><path d=\"M 107.2 109.8 Q 107.2 107.8 109.4 107.8\"/></g><circle cx=\"108.8\" cy=\"109.5\" r=\"0.6\" fill=\"#555\"/></svg><svg id=\"power\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 74 74\"><line x1=\"37\" y1=\"15\" x2=\"37\" y2=\"27\"/><circle cx=\"37\" cy=\"37\" r=\"33\"/><path d=\"M 48 22 A 18 18 0 1 1 26 22\"/></svg><svg id=\"settings\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 74 74\"><path d=\"M30 3 A 37 37 0 0 1 44 3 L 44 13 A 25 25 0 0 1 54.5 20 L 63 14 A 37 37 0 0 1 70 25.5 L 61 31 A 25 25 0 0 1 61 42.5 L 70 48.5 A 37 37 0 0 1 63 60 L 54.5 54 A 25 25 0 0 1 44 61 L 44 71 A 37 37 0 0 1 30 71 L 30 61 A 25 25 0 0 1 19.5 54 L 11 60 A 37 37 0 0 1 4 48.5 L 13 42.5 A 25 25 0 0 1 13 31 L 4 25.5 A 37 37 0 0 1 11 14 L 19.5 20 A 25 25 0 0 1 30 13 Z\"/><circle cx=\"37\" cy=\"37\" r=\"12\"/></svg></div><div id=\"pageSettings\" class=\"page\"><div class=\"pageContent\"><div class=\"pageHead\"><svg class=\"title\" viewBox=\"5 0 105 25\" preserveAspectRatio=\"xMidYMid slice\" role=\"img\"><text x=\"07\" y=\"10\">E</text><text x=\"17\" y=\"10\">W</text><text x=\"27\" y=\"10\">F</text><text x=\"37\" y=\"10\" class=\"g\">G</text><text x=\"47\" y=\"10\" class=\"g\">R</text><text x=\"57\" y=\"10\" class=\"g\">R</text><text x=\"67\" y=\"10\" class=\"g\">D</text><text x=\"77\" y=\"10\">C</text><text x=\"87\" y=\"10\" class=\"g\">S</text><text x=\"97\" y=\"10\">A</text><text x=\"107\" y=\"10\">J</text><text x=\"07\" y=\"20\">N</text><text x=\"17\" y=\"20\" class=\"g\">W</text><text x=\"27\" y=\"20\" class=\"g\">O</text><text x=\"37\" y=\"20\" class=\"g\">R</text><text x=\"47\" y=\"20\" class=\"g\">D</text><text x=\"57\" y=\"20\">U</text><text x=\"67\" y=\"20\" class=\"g\">C</text><text x=\"77\" y=\"20\" class=\"g\">L</text><text x=\"87\" y=\"20\" class=\"g\">O</text><text x=\"97\" y=\"20\" class=\"g\">C</text><text x=\"107\" y=\"20\" class=\"g\">K</text></svg></div><div class=\"pageBody\"><label for=\"color\">Weli Farb wosch?</label><input type=\"color\" id=\"color\" name=\"head\" value=\"#ffffff\"></div><div class=\"pageBody\"><label>Cha mi nid entscheide. Chli vo auem.</label><svg id=\"rainbowMode\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 70 70\"><path transform=\"scale(0.9) translate(5,5)\" d=\"M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z\"/><path class=\"hide\" transform=\"scale(0.85) translate(5,5)\" d=\"M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z\"/></svg></div><div class=\"pageBody\"><label class=\"w100\" for=\"speed\">Wie schnäu?</label><input type=\"range\" id=\"speed\" class=\"slider\" min=\"50\" max=\"2000\"><label>gmüetlech</label><label>jufle</label></div><div class=\"pageBody\"><label>Ir Nacht chli weniger häu.</label><svg id=\"darkMode\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 70 70\"><path class=\"hide\" transform=\"scale(0.9) translate(5,5)\" d=\"M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z\"/><path transform=\"scale(0.85) translate(5,5)\" d=\"M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z\"/></svg></div><div id=\"snakeBody\" class=\"pageBody\"><label>Schnäu e Rundi Snake spile.</label><svg id=\"playSnake\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"-2 -1 12 16\"><path d=\"M2 2 L9 7 L2 12 Z\" stroke-width=\"1.4\"/></svg></div><div id=\"mastermindBody\" class=\"pageBody\"><label>Oder hurti es Mastermind.</label><svg id=\"playMastermind\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"-2 -1 12 16\"><path d=\"M2 2 L9 7 L2 12 Z\" stroke-width=\"1.4\"/></svg></div><div class=\"pageFooter\"><p class=\"popup-content\">Handgmachti Software us Bärn</p><p class=\"popup-content\"><span>Gérard&nbsp;Tyedmers</span><span><svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 -5 160 170\" stroke=\"#fff\" fill=\"none\" stroke-width=\"10\"><circle cx=\"80\" cy=\"80\" r=\"70\"/><path d=\"M27 32c7 20 93 43 121 28M13 60c-3 30 117 60 135 35M16 106c16 19 84 39 112 24M100 13C34 3 10 130 65 148M100 13C70 33 45 118 65 148M100 13c13 22 5 112-35 135M100 13c60 35 20 147-35 135\"/></svg>&nbsp;<a href=\"https://grrd.ch\">grrd.ch</a></span><span><svg xmlns=\"http://www.w3.org/2000/svg\" width=\"30px\" height=\"24px\" viewBox=\"0 0 222 179\" stroke=\"#fff\" fill=\"none\" stroke-width=\"10\" stroke-linecap=\"round\"><g transform=\"translate(-10,10) rotate(-6)\"><rect x=\"15\" y=\"25\" rx=\"10\" ry=\"10\" width=\"192\" height=\"129\"/><path d=\"M15 40 C131 125, 91 125, 207 40\"/><line x1=\"15\" y1=\"134\" x2=\"77\" y2=\"90\"/><line x1=\"207\" y1=\"134\" x2=\"145\" y2=\"90\"/></g></svg>&nbsp;<a href=\"mailto:grrd@gmx.net\">grrd@gmx.net</a></span></p></div></div><svg id=\"settingsClose\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 70 70\"><path transform=\"scale(0.85) translate(5,5)\" d=\"M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z\"/></svg></div><div id=\"pageSnake\" class=\"page\"><div class=\"pageContent\"><div class=\"pageHead\"><svg class=\"title\" viewBox=\"5 0 105 25\" preserveAspectRatio=\"xMidYMid slice\" role=\"img\"><text x=\"07\" y=\"10\">E</text><text x=\"17\" y=\"10\">W</text><text x=\"27\" y=\"10\">F</text><text x=\"37\" y=\"10\" class=\"g\">G</text><text x=\"47\" y=\"10\" class=\"g\">R</text><text x=\"57\" y=\"10\" class=\"g\">R</text><text x=\"67\" y=\"10\" class=\"g\">D</text><text x=\"77\" y=\"10\">C</text><text x=\"87\" y=\"10\" class=\"g\">S</text><text x=\"97\" y=\"10\">A</text><text x=\"107\" y=\"10\">J</text><text x=\"07\" y=\"20\">M</text><text x=\"17\" y=\"20\" class=\"g\">S</text><text x=\"27\" y=\"20\" class=\"g\">N</text><text x=\"37\" y=\"20\" class=\"g\">A</text><text x=\"47\" y=\"20\" class=\"g\">K</text><text x=\"57\" y=\"20\" class=\"g\">E</text><text x=\"67\" y=\"20\">X</text><text x=\"77\" y=\"20\">L</text><text x=\"87\" y=\"20\">B</text><text x=\"97\" y=\"20\">D</text><text x=\"107\" y=\"20\">K</text></svg></div><svg id=\"control\" viewBox=\"0 0 115 110\" preserveAspectRatio=\"xMidYMid slice\" role=\"img\"><path data-num=\"1\" class=\"snakeButton\" transform=\"scale(3.8) translate(8,10) rotate(270)\" d=\"M2 2 L9 7 L2 12 Z\"/><path data-num=\"2\" class=\"snakeButton\" transform=\"scale(3.8) translate(20,7.5)\" d=\"M2 2 L9 7 L2 12 Z\"/><path data-num=\"3\" class=\"snakeButton\" transform=\"scale(3.8) translate(22,19) rotate(90)\" d=\"M2 2 L9 7 L2 12 Z\"/><path data-num=\"4\" class=\"snakeButton\" transform=\"scale(3.8) translate(10,21.5) rotate(180)\" d=\"M2 2 L9 7 L2 12 Z\"/></svg></div><svg id=\"exitSnake\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 70 70\"><path transform=\"scale(0.9) translate(5,5)\" d=\"M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z\"/></svg><span id=\"scoreSnake\"></span></div><div id=\"pageMastermind\" class=\"page\"><div class=\"pageContent\"><div class=\"pageHead\"><svg class=\"title\" viewBox=\"5 0 105 25\" preserveAspectRatio=\"xMidYMid slice\" role=\"img\"><text x=\"07\" y=\"10\">E</text><text x=\"17\" y=\"10\">W</text><text x=\"27\" y=\"10\">F</text><text x=\"37\" y=\"10\" class=\"g\">G</text><text x=\"47\" y=\"10\" class=\"g\">R</text><text x=\"57\" y=\"10\" class=\"g\">R</text><text x=\"67\" y=\"10\" class=\"g\">D</text><text x=\"77\" y=\"10\">C</text><text x=\"87\" y=\"10\" class=\"g\">S</text><text x=\"97\" y=\"10\">A</text><text x=\"107\" y=\"10\">J</text><text x=\"07\" y=\"20\" class=\"g\">M</text><text x=\"17\" y=\"20\" class=\"g\">A</text><text x=\"27\" y=\"20\" class=\"g\">S</text><text x=\"37\" y=\"20\" class=\"g\">T</text><text x=\"47\" y=\"20\" class=\"g\">E</text><text x=\"57\" y=\"20\" class=\"g\">R</text><text x=\"67\" y=\"20\" class=\"g\">M</text><text x=\"77\" y=\"20\" class=\"g\">I</text><text x=\"87\" y=\"20\" class=\"g\">N</text><text x=\"97\" y=\"20\" class=\"g\">D</text><text x=\"107\" y=\"20\">K</text></svg></div><label class=\"popup-content\">Hie chasch d Farb useläse:</label><span class=\"popup-content\"><svg xmlns=\"http://www.w3.org/2000/svg\" class=\"svgButton colorButton\" data-num=\"1\" viewBox=\"0 0 70 70\"><circle cx=\"35\" cy=\"35\" r=\"25\"/></svg><svg xmlns=\"http://www.w3.org/2000/svg\" class=\"svgButton colorButton\" data-num=\"2\" viewBox=\"0 0 70 70\"><circle cx=\"35\" cy=\"35\" r=\"25\"/></svg><svg xmlns=\"http://www.w3.org/2000/svg\" class=\"svgButton colorButton\" data-num=\"3\" viewBox=\"0 0 70 70\"><circle cx=\"35\" cy=\"35\" r=\"25\"/></svg><svg xmlns=\"http://www.w3.org/2000/svg\" class=\"svgButton colorButton\" data-num=\"4\" viewBox=\"0 0 70 70\"><circle cx=\"35\" cy=\"35\" r=\"25\"/></svg><svg xmlns=\"http://www.w3.org/2000/svg\" class=\"svgButton colorButton\" data-num=\"5\" viewBox=\"0 0 70 70\"><circle cx=\"35\" cy=\"35\" r=\"25\"/></svg><svg xmlns=\"http://www.w3.org/2000/svg\" class=\"svgButton colorButton\" data-num=\"6\" viewBox=\"0 0 70 70\"><circle cx=\"35\" cy=\"35\" r=\"25\"/></svg></span><label class=\"popup-content\">Hie muesch di Versuech iigäh:</label><span class=\"popup-content\"><svg xmlns=\"http://www.w3.org/2000/svg\" class=\"svgButton codeButton\" viewBox=\"0 0 70 70\"><circle cx=\"35\" cy=\"35\" r=\"25\"/></svg><svg xmlns=\"http://www.w3.org/2000/svg\" class=\"svgButton codeButton\" viewBox=\"0 0 70 70\"><circle cx=\"35\" cy=\"35\" r=\"25\"/></svg><svg xmlns=\"http://www.w3.org/2000/svg\" class=\"svgButton codeButton\" viewBox=\"0 0 70 70\"><circle cx=\"35\" cy=\"35\" r=\"25\"/></svg><svg xmlns=\"http://www.w3.org/2000/svg\" class=\"svgButton codeButton\" viewBox=\"0 0 70 70\"><circle cx=\"35\" cy=\"35\" r=\"25\"/></svg><svg id=\"sendMastermind\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 70 70\"><path transform=\"scale(0.85) translate(5,5)\" d=\"M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z\"/></svg></span></div><svg id=\"exitMastermind\" class=\"svgButton\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 70 70\"><path transform=\"scale(0.9) translate(5,5)\" d=\"M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z\"/></svg><span id=\"scoreMastermind\"></span></div>");
              // Script
              client.println("<script>!function(){\"use strict\";let e,t,s,a=1,n=1,i=0,c=255,o=0,d=0,r=0,l=0,u=\"1\",m=0,g=0,w=0;function p(e){return document.getElementById(e)}function L(e){return localStorage.getItem(e)}function f(e,t){return localStorage.setItem(e,t)}const v=p(\"clock\"),h=p(\"pageClock\"),M=p(\"pageSettings\"),y=p(\"pageSnake\"),E=p(\"color\"),b=p(\"speed\");function k(){i&&(c&&!d?(c-=1,o+=1):o?(o-=1,d+=1):(d-=1,c+=1),A(\"rgb(\"+c+\", \"+o+\", \"+d+\")\"),setTimeout(k,b.value/10))}function B(e){a=e,a?document.getElementsByTagName(\"body\")[0].classList.remove(\"off\"):document.getElementsByTagName(\"body\")[0].classList.add(\"off\"),s=-1}function A(e){document.documentElement.style.setProperty(\"--main-color\",e)}function S(e){e!==i&&(p(\"rainbowMode\").children[0].classList.toggle(\"hide\"),p(\"rainbowMode\").children[1].classList.toggle(\"hide\")),i=e,i?k():A(E.value)}function _(e){e!==n&&(p(\"darkMode\").children[0].classList.toggle(\"hide\"),p(\"darkMode\").children[1].classList.toggle(\"hide\")),n=e}function N(){let e=parseInt(E.value.substring(1,3),16),t=parseInt(E.value.substring(3,5),16),s=parseInt(E.value.substring(5,7),16);if(f(\"wc_color\",E.value),f(\"wc_rainbow\",i),f(\"wc_darkmode\",n),f(\"wc_speed\",b.value.toString()),window.location.href.includes(\"192.168.\")||window.location.href.includes(\".local\")){let c=new XMLHttpRequest;c.open(\"GET\",\"/update_params?red=\"+e+\"&green=\"+t+\"&blue=\"+s+\"&rainbow=\"+i+\"&darkmode=\"+n+\"&speed=\"+b.value+\"&power=\"+a,!0),c.send()}}function T(e){let t=new XMLHttpRequest;t.onreadystatechange=function(){4===this.readyState&&200===this.status&&(r=10*(parseInt(t.responseText)-3),r>l&&(l=r,f(\"wc_score\",l)),p(\"scoreSnake\").innerHTML=\"Score: \"+r+\" / High-Score : \"+l)},t.open(\"GET\",\"snake?dir=\"+e,!0),t.send()}function I(e){let t=\"\";if(1===e)t=\"mastermind?c4=0\",x(),C();else if(2===e)t=\"mastermind?c4=7\",x();else{if(console.log(document.querySelectorAll(\"[data-num='1'], [data-num='2'], [data-num='3'], [data-num='4'], [data-num='5'], [data-num='6']\").length),document.querySelectorAll(\"[data-num='1'], [data-num='2'], [data-num='3'], [data-num='4'], [data-num='5'], [data-num='6']\").length<14)return void C(\"Muesch zersch aues uswähle.\");t=\"mastermind?c1=\"+document.getElementsByClassName(\"codeButton\")[0].getAttribute(\"data-num\")+\"&c2=\"+document.getElementsByClassName(\"codeButton\")[1].getAttribute(\"data-num\")+\"&c3=\"+document.getElementsByClassName(\"codeButton\")[2].getAttribute(\"data-num\")+\"&c4=\"+document.getElementsByClassName(\"codeButton\")[3].getAttribute(\"data-num\"),x()}let s=new XMLHttpRequest;s.onreadystatechange=function(){if(4===this.readyState&&200===this.status){let e=JSON.parse(s.responseText);m=e.place,g=e.color,w=e.try,4===m?C(\"Bravo! I \"+w+\" Mau usegfunde.\"):11===w?C(\"Schad, jetz hesch verlore.\"):C()}},s.open(\"GET\",t,!0),s.send()}function x(){Array.from(document.getElementsByClassName(\"codeButton\")).forEach((function(e){e.setAttribute(\"data-num\",\"\")}))}function C(e){p(\"scoreMastermind\").innerHTML=e||\"<svg xmlns='http://www.w3.org/2000/svg' class='svgMsg' viewBox='0 0 70 70'> <circle cx='35' cy='35' r='25' fill='white'/></svg>&nbsp;am richtige Ort&nbsp;<svg xmlns='http://www.w3.org/2000/svg' class='svgMsg' viewBox='0 0 70 70'> <circle cx='35' cy='35' r='25' fill='cornflowerblue'/></svg>&nbsp;di richtigi Farb\"}if(setInterval((function(){e=new Date,n&&(e.getHours()>=22||e.getHours()<7)?document.getElementsByTagName(\"body\")[0].classList.add(\"dark\"):document.getElementsByTagName(\"body\")[0].classList.remove(\"dark\"),s!==e.getMinutes()&&(s=e.getMinutes(),v.classList.remove(...v.classList),0!==a&&(s>=55?v.classList.add(\"M5\",\"MV\"):s>=50?v.classList.add(\"M10\",\"MV\"):s>=45?v.classList.add(\"M15\",\"MV\"):s>=40?v.classList.add(\"M20\",\"MV\"):s>=35?v.classList.add(\"M5\",\"MA\",\"M30\"):s>=30?v.classList.add(\"M30\"):s>=25?v.classList.add(\"M5\",\"MV\",\"M30\"):s>=20?v.classList.add(\"M20\",\"MA\"):s>=15?v.classList.add(\"M15\",\"MA\"):s>=10?v.classList.add(\"M10\",\"MA\"):s>=5&&v.classList.add(\"M5\",\"MA\"),t=e.getHours(),s>=25&&(t+=1),t%=12,v.classList.add(\"H\"+t.toString()),v.classList.add(\"M\"+(s%5).toString())))}),100),p(\"power\").addEventListener(\"click\",(function(){B(1-a),N()})),p(\"settings\").addEventListener(\"click\",(function(){document.activeElement.blur(),h.classList.remove(\"swipe-out-right\"),M.classList.remove(\"swipe-in-left\"),h.classList.add(\"swipe-out\"),M.classList.add(\"swipe-in\")})),p(\"settingsClose\").addEventListener(\"click\",(function(){N(),h.classList.remove(\"swipe-out\"),M.classList.remove(\"swipe-in\"),M.classList.remove(\"swipe-out-right\"),h.classList.add(\"swipe-out-right\"),M.classList.add(\"swipe-in-left\")})),p(\"playSnake\").addEventListener(\"click\",(function(){document.activeElement.blur(),M.classList.remove(\"swipe-out-right\"),y.classList.remove(\"swipe-in-left\"),M.classList.add(\"swipe-out\"),y.classList.add(\"swipe-in\"),T(5)})),p(\"exitSnake\").addEventListener(\"click\",(function(){M.classList.remove(\"swipe-out\"),y.classList.remove(\"swipe-in\"),M.classList.add(\"swipe-out-right\"),y.classList.add(\"swipe-in-left\"),T(6)})),p(\"playMastermind\").addEventListener(\"click\",(function(){document.activeElement.blur(),M.classList.remove(\"swipe-out-right\"),pageMastermind.classList.remove(\"swipe-in-left\"),M.classList.add(\"swipe-out\"),pageMastermind.classList.add(\"swipe-in\"),I(1)})),p(\"exitMastermind\").addEventListener(\"click\",(function(){M.classList.remove(\"swipe-out\"),pageMastermind.classList.remove(\"swipe-in\"),M.classList.add(\"swipe-out-right\"),pageMastermind.classList.add(\"swipe-in-left\"),I(2)})),p(\"sendMastermind\").addEventListener(\"click\",I),Array.from(document.getElementsByClassName(\"snakeButton\")).forEach((function(e){e.addEventListener(\"click\",(function(e){T(e.target.getAttribute(\"data-num\"))}))})),Array.from(document.getElementsByClassName(\"colorButton\")).forEach((function(e){e.addEventListener(\"click\",(function(e){Array.from(document.getElementsByClassName(\"colorButton\")).forEach((function(e){e.classList.remove(\"g\")})),e.target.classList.add(\"g\"),u=e.target.getAttribute(\"data-num\")}))})),Array.from(document.getElementsByClassName(\"codeButton\")).forEach((function(e){e.addEventListener(\"click\",(function(e){e.target.setAttribute(\"data-num\",u),C()}))})),document.onkeydown=function(e){let t=0;switch(e.key){case\"ArrowUp\":t=1;break;case\"ArrowRight\":t=2;break;case\"ArrowDown\":t=3;break;case\"ArrowLeft\":t=4}t&&(T(t),p(\"control\").children[t-1].classList.add(\"g\"),setTimeout((function(){p(\"control\").children[t-1].classList.remove(\"g\")}),200))},E.addEventListener(\"change\",(e=>{A(E.value)}),!1),p(\"rainbowMode\").addEventListener(\"click\",(e=>{S(1-i)})),p(\"darkMode\").addEventListener(\"click\",(e=>{_(1-n)})),L(\"wc_color\")&&(E.value=L(\"wc_color\"),A(E.value)),L(\"wc_rainbow\")&&S(parseInt(L(\"wc_rainbow\"))),L(\"wc_darkmode\")&&_(parseInt(L(\"wc_darkmode\"))),L(\"wc_speed\")&&(b.value=parseInt(L(\"wc_speed\"))),L(\"wc_score\")&&(l=L(\"wc_score\")),p(\"iphone\").href=p(\"icon\").href,window.location.href.includes(\"192.168.\")||window.location.href.includes(\".local\")){let e=new XMLHttpRequest;e.onreadystatechange=function(){if(4===this.readyState&&200===this.status){let n=JSON.parse(e.responseText);n.rainbow||(E.value=(t=n.red,s=n.green,a=n.blue,\"#\"+(1<<24|t<<16|s<<8|a).toString(16).slice(1))),A(E.value),_(n.darkmode),S(n.rainbow),B(n.power),b.value=n.speed}var t,s,a},e.open(\"GET\",\"get_params\",!0),e.send()}else p(\"snakeBody\").classList.add(\"hide\"),p(\"mastermindBody\").classList.add(\"hide\")}();</script></body></html>");
            }

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
  MDNS.update();

  // sleep and return when power off
  if (power == 0) {
    delay(500);
    return;
  }

  if (inSnake) {
    if (snakeWait > 0) {
      snakeWait--;
    } else {
      snakeNext = -1;
      snakeWait = snakeSpeed;
      if (snakeDir == 1) {
        // move snake up
        snakeNext = snake[0] - 1 - 2 * (snake[0] % 11);
        if (snakeNext < 0) {
          snakeNext = -3;
        }
      } else if (snakeDir == 2) {
        // move snake right
        snakeNext = snake[0] + 1 - 2 * ((snake[0] / 11) % 2);
        if (floor(snakeNext / 11) != snake[0] / 11) {
          snakeNext = -3;
        }
      } else if (snakeDir == 3) {
        // move snake down
        snakeNext = snake[0] + 1 + 2 * (10 - snake[0] % 11);
        if (snakeNext > 120) {
          snakeNext = -3;
        }
      } else if (snakeDir == 4) {
        // move snake left
        snakeNext = snake[0] - 1 + 2 * ((snake[0] / 11) % 2);
        if (floor(snakeNext / 11) != snake[0] / 11) {
          snakeNext = -3;
        }
      }
      if (snakeNext == snake[1]) {
        // ignore move backwards
        snakeDir = snakePrevDir;
        snakeNext = -2;
      }
      for (int i = snakeLen - 1; i > 1; i--) {
        if (snakeNext == snake[i]) {
          // bite own tail
          snakeNext = -3;
        }
      }
      if (snakeNext == snakeSnack) {
        // found snack
        snakeLen++;
        snake[snakeLen] = -1;
        setSnack();
        snakeSpeed = snakeSpeed - 175;
      }

      if (snakeNext >= 0) {
        // move snake on step forward
        for (int i = snakeLen - 1; i > 0; i--) {
          snake[i] = snake[i-1];
        }
        snake[0] = snakeNext;
        blank();
        lightup(snake, Green);
        pixels.setPixelColor(snakeSnack, Red);
        pixels.show();
      }

      if (snakeNext == -3) {
        // game over
        chase(Red);
        inSnake = false;
        lastMinuteWordClock = 61;
      }

    }
  } else if (inSnake) {

  } else if (rainbow == 1) {
    if (rainbowWait > 0) {
      rainbowWait--;
    } else {
      rainbowWait = rainbowSpeed;
      if (rgbRed > 0 && !rgbBlue > 0) {
          rgbRed--;
          rgbGreen++;
      } else if (rgbGreen > 0) {
          rgbGreen--;
          rgbBlue++;
      } else {
          rgbBlue--;
          rgbRed++;
      }
      colorDay  = Adafruit_NeoPixel::Color(rgbRed / 5, rgbGreen / 5, rgbBlue / 5);
      colorNight  = Adafruit_NeoPixel::Color(rgbRed / 25, rgbGreen / 25, rgbBlue / 25);
      lastMinuteWordClock = 61;
    }
  }

  if (timeStatus() != timeNotSet && !inSnake && !inMastermind) {
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
