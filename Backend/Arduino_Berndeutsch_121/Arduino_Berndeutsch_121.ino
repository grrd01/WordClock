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

// ToDo: WordGuessr: ungültige Worte in der Wortliste erkennen

#include <Arduino.h>
#include <ESP8266WiFi.h>        // v2.4.2
#include <ESP8266mDNS.h>
#include <WiFiManager.h>        // v2.0.3-alpha
#include <WiFiUdp.h>
#include <TimeLib.h>            // v1.6.1
#include <Timezone.h>           // v1.2.4
#include <Adafruit_NeoPixel.h>  // v1.10.4
#include <pgmspace.h>

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
int ghost = 1;
int rainbow = 0;
int rainbowSpeed = 200;
int rainbowWait = 200;


// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

int wordClockMinute = 62;
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

// WordGuessr variables
// äöü durch aou ersetzen, damit Zugriff über Byte-Index möglich ist (äöü brauchen 2 Bytes, aou nur eins)
// ÄSIISCHRFÜFMÄZYTUTREIVZWÄNZGQDVORZTIBUAHDBAKEISQZWÖIDRÜTIFÜFIREIVZSÄCHSISIBNIFOINÜNITHCACZÄNIEUFIDIXXIFUÖWZGKOIDB..T..HY.
const char* wordGuessrLetters = "aSIISCHRFuFMaZYTUTREIVZWaNZGQDVORZTIBUAHDBAKEISQZWoIDRuTIFuFIREIVZSaCHSISIBNIFOINuNITHCACZaNIEUFIDIXXIFUoWZGKOIDB..T..HY.";
char wordGuessrLettersCopy[121];

const char wordGuessrWords0[] PROGMEM = "AARE ABEND ABENTEUER ACHSE ACHT ACKER ADER ADRIA AFRIKA AGENT AKTION AMEISE AMERIKA AMOR AMT ANANAS ANFANG ANGEBOT ANGST ANTWORT ANZUG ARBEIT ARCHIV ARGENTINIEN ÄRGER ARM ARMBAND ARZT ASTRONAUT ATHEN ATMEN AUGE AUGUST AUSDAUER AUSGANG AUTO AUTOBAHN AVOCADO BACH BACKE BADEN BADEWANNE BAHN BANANE BAND BANK BÄR BART BATTERIE BAUCH BAUER BAUM BECHER BEDINGUNG BEERE BEGINN BEIN BEITRAG BEKANNTSCHAFT BERATUNG BERG BERICHT BERN BERUF BESCHWERDE";
const char wordGuessrWords1[] PROGMEM = "BESUCH BETON BETRAG BETT BEWEIS BEWERTUNG BIBER BIENE BIER BIRNE BISON BODEN BOGEN BOHNE BOMBE BRAUE BRAUN BREI BREIT BRIEF BRONZE BROT BRÜCKE BRUST BUCH BUCHE BUCHSTABE BÜHNE BUND BURG BÜRSTE BUSCH BUTTER CHINA DACH DACHS DAME DÄNEMARK DANK DARM DAUER DAUMEN DECKE DETEKTIV DICK DIEB DIENST DING DOKTOR DOKUMENT DOMINO DONAU DONNER DOOF DORF DRACHE DRAMA DREI DREIECK DREIRAD DROMEDAR DÜNN DUNST EBBE EBENE";
const char wordGuessrWords2[] PROGMEM = "ECHSE ECKE EHE EI EICHE EIDECHSE EIER EIGENSCHAFT EINKAUF EINS EIS EISBÄR EISHOCKEY ENDE ENERGIE ENTE ERBSE ERDE ERFINDUNG ERINNERUNG ERWARTUNG ESCHE ESSEN ESTRICH EWIGKEIT EXTREM FADEN FAHRRAD FANTASIE FARBE FASS FEBRUAR FEE FEIGE FENSTER FERNSEHER FEST FESTUNG FEUER FEUERWEHR FINGER FINK FIRMA FISCH FISCHER FONDUE FORM FORMAT FOTOGRAFIE FRAGE FRAU FREIHEIT FREITAG FREUDE FREUND FRIEDEN FRISUR FROSCH FROST FRUCHT FRÜHSTÜCK FUCHS FÜNF FUNKTION FUSS";
const char wordGuessrWords3[] PROGMEM = "GANG GANS GARTEN GAS GASSE GAST GEBÄUDE GEBIET GEBISS GEBURT GEDANKE GEDICHT GEFAHR GEFÄNGNIS GEGENSTAND GEHEIMNIS GEHEN GEHIRN GEIER GEIST GEMÜSE GENF GERICHT GESANG GESCHÄFT GESCHICHTE GESCHMACK GESICHT GETREIDE GEWICHT GEWINN GIRAFFE GOTT GRABEN GRAD GRAS GRENZE GROSS GRUBE GRÜN GRUND GUT HAAR HAHN HAI HAMSTER HAND HASE HASS HAUS HAUT HECHT HEFT HEIMAT HEISS HERBST HERDE HERZ HIMBEERE HINTEN HINWEIS HIRSCH HIRT HITZE HOBBY";
const char wordGuessrWords4[] PROGMEM = "HOCH HOCHHAUS HOCHZEIT HOFFNUNG HONIG HOSE HÜFTE HUHN HUND HUNDERT HUT HÜTTE HYÄNE INDIANER INDIEN INFORMATION INTERESSE INTERNET IRAK IRAN KÄFER KAFFEE KAIRO KAISER KAKAO KAMERA KAMIN KANADA KÄNGURU KANTON KANU KAROTTE KARTE KÄSE KASSE KATZE KAUF KENIA KERN KERZE KETTE KIEFER KIES KIND KINO KIRCHE KIRSCHE KISSEN KIWI KNAST KNIE KNOCHEN KOBRA KOFFER KOMET KONFITÜRE KONGO KÖNIG KONSTANT KONTAKT KONZERT KOREA KOSTEN KRAFT KRAFTWERK";
const char wordGuessrWords5[] PROGMEM = "KRAKE KRANICH KRATER KREIS KREUZ KRIEG KRIMI KRISE KRITIK KRÖTE KÜCHE KUH KÜKEN KUNDE KUNST KÜRBIS KURS KUSS MÄDCHEN MADRID MAGEN MAI MAIS MANDARINE MANGO MANN MARKE MARKT MARS MÄRZ MASCHINE MATRATZE MAUER MAUS MEER MEINUNG MEISE MEISTER MENGE MENSCH MESSER MESSING METER METRO MEXIKO MINUTE MITTAG MITTE MODE MONAT MOND MONITOR MÖRDER MORGEN MÖVE MUND MÜNSTER MÜNZE MUSIK MUT MUTTER MÜTZE NACHT NAH NÄHE";
const char wordGuessrWords6[] PROGMEM = "NAME NARBE NASE NASHORN NASS NATUR NETZ NEUN NIERE NIGERIA NORDEN NORWEGEN NOVEMBER NUSS OBST OCHSE OHR OHRRING OKTOBER OMA ORANGE ORDNER OSTEN QUITTE RABE RAD RADIO RADIUS RAKETE RASEN RATTE RÄUBER RECHT REDEN REGEN REGION REH REICH REIM REIS REISE RENNEN RENTIER REST RICHTER RICHTIG RITTER ROBBE ROBOTER ROCHEN ROM ROMAN ROSA ROT RUANDA RÜCKEN RUHE RÜHREI RUNDE SACHE SAFT SAMSTAG SAND SATURN SATZ";
const char wordGuessrWords7[] PROGMEM = "SCHACH SCHAF SCHATTEN SCHIFF SCHMUCK SCHNAUZ SCHNECKE SCHNEE SCHNEIDER SCHÖN SCHRANK SCHRAUBE SCHRIFT SCHRITT SCHROTT SCHUH SCHUTZ SCHWAN SCHWEIN SCHWEIZ SCHWERT SECHS SEE SEHNE SEITE SEKUNDE SEMESTER SENDUNG SENSATION SICHT SIEBEN SIEG SKI SOCKE SOFA SOHN SONNE SORGE STAAT STADT STAND STAR START STECKER STEHEN STEIN STEINBOCK STERN STIER STIFT STIRN STOFF STORCH STRAND STRASSE STRAUSS STROH STROM STÜCK STUDIE STUNDE STURM SÜDEN SYRIEN SZENE";
const char wordGuessrWords8[] PROGMEM = "TAG TANDEM TANKER TANNE TANSANIA TANTE TANZ TASTATUR TÄTER TAUCHER TAUSEND TEE TEENAGER TEER TEICH TENNIS TERMIN TESSIN TEST TEUER TEXT THEATER THRON TIEF TIER TIGER TINTE TISCH TOCHTER TOD TOKYO TOMATE TOURIST TRAKTOR TRAM TRANK TRAUBE TRAUER TRAUM TRICK TRINKEN TROCKEN TROTTOIR TRUTHAHN TUCH TÜR TÜRKEI UGANDA UHR UHU UKRAINE UMFANG UNGARN UNIVERSUM UNKRAUT URI URIN URSACHE VATER VENE VENEDIG VENUS VERBAND VEREIN VERSE";
const char wordGuessrWords9[] PROGMEM = "VERTRAG VIDEO VIER VIETNAM VORHANG VORNE WADE WAGEN WAND WANDERUNG WARM WÄRME WASSER WEG WEIHNACHT WEIN WEIT WEIZEN WERBUNG WERK WERKZEUG WERT WESTEN WETTER WIEN WIND WINTER WIRKUNG WITZ WITZIG WOCHE WORT WUNDE WUNDER WUNSCH WURM WURST ZANGE ZAUN ZEBRA ZEH ZEHN ZEICHEN ZEICHNUNG ZEIT ZEITUNG ZEMENT ZIEGE ZINS ZITRONE ZUCKER ZUG ZUKUNFT ZUNGE ZÜRICH ZUSTAND ZWECK ZWEI ";

const char* const wordGuessrWordsAll[] PROGMEM = {wordGuessrWords0, wordGuessrWords1, wordGuessrWords2, wordGuessrWords3, wordGuessrWords4, wordGuessrWords5, wordGuessrWords6, wordGuessrWords7, wordGuessrWords8, wordGuessrWords9};
char wordGuessrWordsBuffer[480]; // Buffer to load one of the 10 wordlists from progmem to ram; must be long enough for longest list

String wordGuessrActiveWord = "";
int wordGuessrActiveWordIndex[15] = {}; // max 15 letters
int wordGuessrScore = 0;
unsigned long wordGuessrAlert = 0;
unsigned long wordGuessrStart = 0;
int wordGuessrHint = 0;
bool inWordGuessr = false;

// Ghost variables
int ghostHour = 0;
int ghostMinute = 0;
int ghostStep = 0;
int ghostChange = 1;
static int WordGhost[] = {5, 6, 7, 17, 16, 15, 25, 26, 27, 28, 29, 40, 38, 36, 44, 47, 49, 51, 53, 54, 65, 64, 63, 62, 61, 59, 58, 57, 56, 55, 67, 68, 69, 70, 72, 73, 74, 75, 85, 84, 83, 82, 81, 80, 79, 91, 92, 93, 94, 95, 105, 104, 103, 102, 116, 117, 118, -1};
static int WordGhostEyes[] = {39, 48, 37, 50, -1};
bool inGhost = false;

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
 * Function to scale brightness by a factor
 * @param color Adafruit_NeoPixel-Color to display
 * @param factor float to dim color, 0.5 for 50% brightness
 */
uint32_t dimColor(uint32_t color, float factor) {
  uint8_t r = (color >> 16) & 0xFF;  // Extract the red component
  uint8_t g = (color >> 8) & 0xFF;   // Extract the green component
  uint8_t b = color & 0xFF;          // Extract the blue component

  // Scale each color component by the brightness factor
  r = (uint8_t)(r * factor);
  g = (uint8_t)(g * factor);
  b = (uint8_t)(b * factor);

  // Return the new color
  return Adafruit_NeoPixel::Color(r, g, b);
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
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 2500) {
    int size = ntpUDP.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      ntpUDP.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long) packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long) packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long) packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long) packetBuffer[43];
      setSyncInterval(120);
      setWifiStatus(Green, 250);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  setWifiStatus(Red, 500000000);
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
  ntpUDP.begin(localPort);
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
  // if it does not connect it starts an access point with the specified name (wordclock)
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect(version);
  // or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

  // if you get here you have connected to the WiFi
  Serial.println(F("Connected."));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());
  // Display local ip address on clockface by looping through every character.
  String localIP = WiFi.localIP().toString();
  for (int i = 0; i < localIP.length(); i++) {
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
    Serial.println(F("DNS gestartet, erreichbar unter: "));
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
 * find a random index of a letter in the wordGuessrLetters, return -1 if letter is not in the word
 * @param letter the letter to find
 * @param allLetters the string to search in
 */
int wordGuessrFindIndex(char letter) {
  int indices[20]; // max 16 occurences of a letter (i has 19)
  int count = 0;
  for (int j = 0; j < 121; j++) {
    if (wordGuessrLettersCopy[j] == letter) {
      indices[count] = j;
      count++;
    }
  }
  if (count == 0) {
    return -1;
  }
  int randomIndex = indices[random(0, count)];
  wordGuessrLettersCopy[randomIndex] = '.';
  return randomIndex;
}

/*
 * select a new word to guess
 */
void wordGuessrNewGuess() {
  randomSeed(micros());

  int listNr = random(10); // Index to randomly load one of ten wordlists from progmem
  strcpy_P(wordGuessrWordsBuffer, (char*)pgm_read_ptr(&(wordGuessrWordsAll[listNr])));

  // Zähle, wie viele Wörter vorhanden sind
  int wordCount = 0;
  char* wordGuessrWords[70];  // Array to store list of words (max 100)

  // use strtok, to separate the words
  char* token = strtok(wordGuessrWordsBuffer, " ");
  while (token != NULL) {
    wordGuessrWords[wordCount++] = token;
    token = strtok(NULL, " ");
  }

  int randomIndex = random(wordCount);
  wordGuessrActiveWord = wordGuessrWords[randomIndex];
  Serial.print(F("New word to guess: "));
  Serial.println(wordGuessrActiveWord);

  // Erzeuge eine Kopie
  String wordGuessrDecodeWord = wordGuessrActiveWord;
  // Ersetze alle Umlaute durch ihre Kleinbuchstaben-Äquivalente
  wordGuessrDecodeWord.replace("Ä", "a");
  wordGuessrDecodeWord.replace("Ö", "o");
  wordGuessrDecodeWord.replace("Ü", "u");

  // Kopiere den Inhalt von wordGuessrLetters in den neuen Array
  strcpy(wordGuessrLettersCopy, wordGuessrLetters);
  for (int i = 0; i < wordGuessrDecodeWord.length(); i++) {
    wordGuessrActiveWordIndex[i] = wordGuessrFindIndex(wordGuessrDecodeWord[i]);
    wordGuessrActiveWordIndex[i + 1] = -1;
  }
  wordGuessrStart = millis();
  wordGuessrHint = 0;
}

/*
 * prepares a new mastermind game
 */
void clearMastermind() {
  wipe();
  inMastermind = true;
  inWordGuessr = false;
  inSnake = false;
  randomSeed(micros());
  mastermindCode[0] = random(1,7);
  mastermindCode[1] = random(1,7);
  mastermindCode[2] = random(1,7);
  mastermindCode[3] = random(1,7);
  mastermindTry = 0;
  mastermindPlace = 0;
  mastermindColor = 0;
  for (int i = 0; i < 11; i++) {
    pixels.setPixelColor(down(0, i), Grey);
    pixels.setPixelColor(down(5, i), Grey);
    pixels.setPixelColor(down(10, i), Grey);
  }
}

/**
 * Main setup to start WordClock
 */
void setup() {
  Serial.begin(115200);
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
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-type:text/plain"));
              client.println(F("Access-Control-Allow-Origin: *"));
              client.println(F("Connection: close"));
              client.println();
              client.println(snakeLen);
              if (extractParameterValue(url, "dir=") > 0 && extractParameterValue(url, "dir=") < 7) {
                snakePrevDir = snakeDir;
                snakeDir = extractParameterValue(url, "dir=");
              }
              if (!inSnake && snakeDir == 5 && power == 1) {
                // start new snake game
                inSnake = true;
                inMastermind = false;
                inWordGuessr = false;
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
              } else if (inMastermind && mastermindCodeTry[3] != 0 && mastermindCodeTry[3] != 7) {
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
                mastermindTry ++;
              }
              if (mastermindPlace == 4) {
                // player won
                for (int i = 0; i < mastermindTry; i++) {
                  pixels.setPixelColor(down(0, i), Green);
                  pixels.setPixelColor(down(5, i), Green);
                  pixels.setPixelColor(down(10, i), Green);
                }
              } else if (mastermindTry == 11) {
                // player lost
                for (int i = 0; i < mastermindTry; i++) {
                  pixels.setPixelColor(down(0, i), Red);
                  pixels.setPixelColor(down(5, i), Red);
                  pixels.setPixelColor(down(10, i), Red);
                }
              }
              if (power == 1) {
                pixels.show();
              }
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-type:application/json"));
              client.println(F("Connection: close"));
              client.println();
              client.println(F("{\"place\":"));
              client.println(mastermindPlace);
              client.println(F(", \"color\":"));
              client.println(mastermindColor);
              client.println(F(", \"try\":"));
              client.println(mastermindTry);
              client.println(F("}"));
            } else if (header.indexOf("wordguessr") >= 0 && power == 1) {
              // Client is playing wordguessr game:
              if (header.indexOf("new") >= 0) {
                if (!inWordGuessr) {
                  // start new wordguessr game
                  inWordGuessr = true;
                  inSnake = false;
                  inMastermind = false;
                  wordGuessrNewGuess();
                  wordGuessrAlert = millis();
                }
                wordGuessrScore = -1;
              } else if (header.indexOf("exit") >= 0 && inWordGuessr) {
                // exit current wordguessr game
                inWordGuessr = false;
                lastMinuteWordClock = 61;
                wordGuessrScore = -2;
              } else if (inWordGuessr) {
                int startIndex = header.indexOf("=") + 1; // Sucht die Position des ersten "="-Zeichens
                int endIndex = header.indexOf(' ', startIndex);
                String word = header.substring(startIndex,endIndex); // Extrahiert den Teil des Strings nach dem "="-Zeichen
                word.replace("%C3%84", "Ä");
                word.replace("%C3%96", "Ö");
                word.replace("%C3%9C", "Ü");
                if (word == wordGuessrActiveWord) {
                  // right guess
                  wordGuessrScore = 1;
                  lightup(wordGuessrActiveWordIndex, Green);
                  pixels.show();
                  wordGuessrNewGuess();
                  wordGuessrAlert = millis() + 100;
                } else {
                  // wrong guess
                  wordGuessrScore = 0;
                  lightup(wordGuessrActiveWordIndex, Red);
                  pixels.show();
                  wordGuessrAlert = millis() + 100;
                }
              }
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-type:application/json"));
              client.println(F("Connection: close"));
              client.println();
              client.println(F("{\"score\":"));
              client.println(wordGuessrScore);
              client.println(F("}"));
            } else if (header.indexOf("update_params") >= 0) {
              // Get new params from client:
              const char *url = header.c_str();
              if (extractParameterValue(url, "ghost=") == 1) {
                ghost = 1;
              } else if (extractParameterValue(url, "ghost=") == 0) {
                ghost = 0;
              }
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
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-type:text/plain"));
              client.println(F("Connection: close"));
              client.println();
              client.println(F("OK"));
            } else if (header.indexOf("get_params") >= 0) {
              // Send current params to client:
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-type:application/json"));
              client.println(F("Connection: close"));
              client.println();
              client.println(F("{\"red\":"));
              client.println(rgbRed);
              client.println(F(", \"green\":"));
              client.println(rgbGreen);
              client.println(F(", \"blue\":"));
              client.println(rgbBlue);
              client.println(F(", \"rainbow\":"));
              client.println(rainbow);
              client.println(F(", \"darkmode\":"));
              client.println(darkMode);
              client.println(F(", \"speed\":"));
              client.println(rainbowSpeed / 4 + 48);
              client.println(F(", \"power\":"));
              client.println(power);
              client.println(F(", \"ghost\":"));
              client.println(ghost);
              client.println(F("}"));
            } else {
              // New connection, send web interface to client:
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-type:text/html"));
              client.println(F("Connection: close"));
              client.println();

              // Display the HTML web page
              // Head
              client.println(F("<!doctype html><html lang='en'><head><meta charset='utf-8'><title>grrd s WordClock</title><link id='icon' rel='icon' href='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAALQAAAC0CAAAAAAYplnuAAAABGdBTUEAALGPC/xhBQAAAAFzUkdCAK7OHOkAAAAJcEhZcwAADsMAAA7DAcdvqGQAAA/zSURBVHja7V17jB7Vdf+dOzPf+7Xep73rxxob27ALtnm4BgMmtES0kaqKtApR6SuUKkKNokS0qVo16kNpaKM2rZq2UVCTCggpadWQtIkaAg4uJjFg2LWNwW9srx/7/t6vmXtP//i+XW/EzJ35WCAQzffXzH7nu/c3957zu+eec88s3YH330cgBB2C/tkCbXI40iHoEHRoiKF6hKBD0O+qIRIrCJIsxHL7NN/FAVqxSp2vD6bnLjrLbMhY925B5sYvf/r2ianP/urgM4p+2iO9dK41YJg5k0Iq0h1dYTiggC1Sp6AZBIa+fZCQsYVraRvSc0aliJx8SZ6tvjB0hAQM5anXgg1r4dpWpNw63e0F2YhagthpNrWg5a67F5+bzv5zxeMROfvxwaMP18gkG+pDdzYfP2B4zshHdy4+EO1/lCg4e3AsbQBAJN4sKs1sOMPXXb7ri5Q95FT6ut64YDhgtjdsxb795PV4fPU1l+8qjhlcPTiSEXbDYRGJRbPzWvUALsy1r880vG1HQVkggGASpCCdlckz1fbNq0YnOp0QzbwCUGtmIrGaXq+f+2pL8WVT+BhAQNtuPravNRG2NFVw0MJCrUVMtVgsVvPpo2DSm5lkOYRULRn0ltiDuPWLOttBR8hPrtUiB6BQ0jboYYjKMZKywSCgVtWAYWYAkgUAZRqsRcLgRczMWpK2DQKYI50tLmXLyDl205bsr4lr72EAkKdeC6Qe/iNtXNcHAGgcmugENDXzacuyEko2a77asevmlh0888cWvS06Hb27pSPqy/8WpQ5Gmhp2NGoZQljxatmnD9WycJZvm58iW7OhZIeGSFyrCSMSiYgUSvoB3Ps8EwD7bEwFNVe9ijT+4zQI4PqxGHcAmgEiKNUkMxNN1Bwt6oknWwRlCG89lQwSDAIYUXDT27IByEN7WouKSR2wB6fijSIBINjFbmHa2j4gWqCJNczbQDzGxADMFJwy9ERjGDqj9RgcIxJboErpTwnU+ugkinlkVoEBRnYAtSn92kkEXYseoOvKTLa8RxURylm+aVVPInNHVCnF6ubVuHhuWTRjejBeI5Y2KpIhkmmq28tmMhJ7P5D7eXPPRc5t+6U4P1egANba6c6FCyKSjDuKDBPNomYhWFjqyG8BH3vynugHby5zIkP44bc997bMDBCztkEvf9qZTcWNCMBOvaS8LYxl1aGaNMlP8Vk9UvrQUCoF8PSex0rsbYiNGpoV1pIi7fLsRpiGYGn77EKHRunw2WARhJXbhzOiNvHKcS0isWV99UBe39SuYP6WxjFF0MWbpRKkNHTepiubovyWd+PBsFiBfXsyDED4ibOI+Cnb8kMI9LaL+oqF8ekQdAg6BP0WHaaQPULQIejQEEP1CEGHoN+vhqjduSiQ7y5iMeZI9F4ALaycla/ro0sUTSdaKZ18wdCjvhyd98mo+qeAvUGzffdHzKN/UdIiadzyQCvFIIvH/uewJjTAkS7MN1s5l0wCtaJO8DLmSok6G+nMHauQ3fk9oU14965pXw5uufFLT+ssPtY2epXNiVpZK3gZdNM10uRtiHLbBjjR256S+imXxvHjDEqsGV75sWNnyPvxuBUX5myOKjOaKWFGYVEnbddogumNZ1eyOLnxmqsO+unfkc8KInPowR3Dtz0c8bNGlc1Rddon9Ferk1apvXl67VZMPCt7blPsyx+GaajTzzpY68ufKttFFT/MC/Fuzwi1F2iWO4cw9swkdgwE5CHJqHEAzOUZLJccPUc6ttPI7ztxCOtu8ksFCHZsB2tvtWqvWP6YS7P+oHjx0xFPM49swfFD8vnbYrd+19Z3senPGJRad4X6wXP6IJvKdJE9x/7jHGu3Q6rWyTLOcleX2l80Xziz6erRl7yzVgxg8+bWzcnv2zpSAKdSxGZ2TvisQcACU5N9njtRj97rMfkCOZNj6NrtY4qV6enp6bkGNnzqBr15pUW+QpmEv7fTrLc/De5APVjeuA641wZ6HPOGoYvaLp5/hEHRwV+4afj+EzPaFFdhzoxa3Reln4LM1nxOIbj/2dgVw8qVres1tzyudSvKYxYBB158aGTTjm9pAuxUmBNyrs/qnvIdah/ny3RXwCtHUbrAABBbY97yZEOrgkIQQJeOjViDjoY/uAJQpZhNZvMioL/UiT/Nzs0D2Pt3TIBa/+ertmzb5+GAtDhJMQCmBNCA9hgHAzQXi+bqddJiZn4rPJ35OarunSnk8/nCy2PI3K6fS7vRaNq0exvKxyx/Dp5RosfggDwd3BBZbduI0wcsAsDy/3Ynrhs+o+liw58owOi/phcHDhi+JEz1/IqIj1rHFieW64F1Wt6axoFZEwDIeOHUyJpb/9Xy0g+BkZH2zeGHm+RvXSIfS6QampQtoXvx2pkI7OVFVlam9ywM2vyeQWu9bXngmJ9pbQJUY/rAkxd0y6G9uDObNUSy4unxLj1WQe77JrreDfSqdOlis/2MIjFkFrxOD1NqZbKlRaVLRb1C0xIjJR1JkC+XuIKGUmQsUXEWQufcLbqTP92N7U+AJEPXgBEGa8IIUzjSIegQdAg6ZI8QdAg6NMRQPULQIeiODZFB70nQ3oBZmAako4JtWJkRWFAssz1P0CqejZsCyq7kbe9OVF+qPVXs1AuayhJeuVCyomS1qAn20iqzHQNolCuiM9Dc22WwdGDE45nJsjdqK9qOtxGlMhc1gcUFQQKlMhc0VQaRdviEkrnipHuuwwO0Gsihmq9JmKlcZGCi4Z3URH2qVZmTSkf7z+rMozJLAGCmk/HeCzrBYpEAYaViWXGhA0NUuSwKl5QAGrXyUKT7vHcP4FYxMOf7u6OJMnkLytaEcX4wk4w0NArSLBgAY3Ygm865ZmjcJ150idolFgDIqM0jYWnLCoiISIi89DkW3xZEnoVWkAQRCcEXa5QTgdWDk1GeV215UbTqgUrqA7MjBatrJTUfjyTciro9QIvmojDJS3qOWijlzBpcJx8WA8DIkmwEGoWaNGLloKARgS0v18P6lCjH2vaVoXJFJypiAgBZ6TQVG0GomqQ0rcDqQQZUi2zahZeO9/kJjq5tK6IqaVN3nFzXwk6yOBVMlZRyn2P3RFG7HImt1QYAptkp4c0eEiATMl+qaIr7uFUgSibsYqkq9Eq9NNXilnZx12mFVh6gVYGqTaRQ4ywg0j2GWdPrEVXOC4hctylqgQtjBGTwxaUJy1AAybMAsDaqn0SQM+usyhh+1VCK4EypvhXiQjDMHDHcS0Tdp72izKQCAMdxHN8qSgIZhTlO9CtfqiNjtsjZFSoY6KShXGfFFbQoN8QKwa31YEmFuhfdARAzNWRTejAtKppsojuqLVZtqzGLLDXcQbPbR86o+CpylFJKqqwJYs9PO1MJOalEr04OraQm7Bk2+5R3cwBLKaV0aGVMzbkKeuRcCrGebLRQdmAksnHhBDAdUc6viPdM+qdgxHw6neqaFxoSXUGAEUlGkc8HX8YBMWn3xGI9CsKALE7XhMafWPjJdDLWXfZ8PFpMydFU3OqteDqnhEymZQBOftK9X6Pf46fVoiQySDVLUzPSe1jiot4+pEhSRhQ818QEau3vyIbJXPUQpATbjuM4zVphyuu0Ao16MzwZBim/7daSbaT2DOnS7aaP4OIPqPONLUFKpp/ItOoZBHrvjRFQ0NcXFG+TuxnGPULQ748IUzjSIegQdAg6ZI8QdAj6vWyI72Jf7U02LdvhfVdfezvJ/dZ8OZ1V75+RdrbdRd8f+/21px5/59SDO9m+BChe4eL230T+x3dvOPq4Eu8MaLnkGC95vNXQVmYrzME2C2vxb54aJyUc24YTRDuarKldoy2uo2KuGVh8HK6cKrx5JBnp9fELFxwCi8yG2PzJBhNTbH165g3XdCLDHvmVVTOVfn7xm+Qze0y5YfPcpOpsN87zf79TLu7kG597Iukyzn2fH9h/PwFc+Nh9xtxn9ptge+tDvU/d61qiRs3dH7mmy4KqrM89VvOJT5d/8Y9iX/9Mb4fqoVLxyzfJtNuRb2OqEdu8+YgJTuzIIHnTsymgfsNqTMdcMTv3/3YvZEHG0yNbtn3+nF45VK5L5MwOdZpyX3+p9d4+68ZRnD+SdJuoyeObcleNGcSbhiGN0TgTG5tRGU+4tdj4nQeSzkt7TtT7d9ze94HIgz7v4YISMLjD3Dh9q2IygPqn16H8lZfd0jWkDt8V3WzHubGtt3lh3fqNJxgrhzE57lLzwOq630rWHvnSvEXym3f94RW77vvL6DJWGA/yUbHubC6XS374d9PyiUfdD/tHx/PYkGKW11ozP+K+rU04mwZwwrW6pP7hQXz3c/WEZUaT//uPBXxwZDkLjNCEk8FXfHIAP/ii5S4hXpvA0GrFA5tx+pl8ZGtT1a/O8OEm3iyr1l2Pmf9sz2rs2y9i1e6K8o5lswLACl5fa2k++uAIxv+67qV4k0fRs9m2R4dw+EcT2DKgsFnkX3GxQ25e1YczhxZKbhovSXG1fzTPuxhKB7rxyTtw/m/PeIqYh+zk5lpte7b8ysRRDI06A1dg4lW3XFhjdRyXFm3POl1Ff+6dcU3r9/6aKP3Ls96va40enMWVZmwUFw4mxu2ubcUrB3D0kpuByYyBykLVK4liE4kEvxOgnVt/LyW/8WhM89ujZ7B28MorcOxsfGwGozSSs8cNvR+DdqSal+NdejoKPPSpQTz1xYiu8fnXdvRvHOxTBxW9fnrl+pEtxtRYxEVZ2ShII2ksmLfMRlEp6V6coABAdR6fNv9gK8Ye0r+6Pnqonh3dHp19OUbFV9G3YwPeeN21wdipCoZ6FjV8YwLn8777hc4Nsf6JOzHxhbNacqHI+JS4ZgvOHDEoNlZJ7liFV4tuKk3W+FkM39wudubMDuGMmf5FgJ3qdOOej5rFf3pWKKWU8nxkOnkam1fjtVmiyPhFXJutjrmbAF3ai8yvd0tmZlX+je04/XTibV8R1c6PZ5pPfNUulUqlUum81xk3qh7BYK72ShTAG8ex2rp40L1ukZLfOIydf7WxVKmWxAP3x+3/Or2cfaIroanqfWtR3PC1hZN0P/x3D08hNl5KY3IsApA8eGcUJ07FPfo5/w9/OnTnxv0nGv3btsf5v78W92cP7ow9ONoP9Oy+7Dr/zQb3X1tjk2mceD0B5tiBwgqM215gjKf4E6PDw0qZQOE7X3C0+S2WygBqOe5kpMk5mZNL7OFcl5dSn9sXdfYSAWQe/PG10/uSnrNuPf36PbuGElSbe/U734v4DHLk0vHYOW8/kDa6PoqZWnLGo1r1PA5kiLRTbb1T3RTpquPoxq/Ut6XfKJ08ZkV8iSNiGXVbdQYarC574Eya19awwsK3rMjn3Qzs2GxERJDtvWL9v8dYXrhsCQThS7yWhYAJVW3xq7n8/0fAAeU6EtKKhVHTEPTPGugwURSCDkGHhhiqRwg6BP2+Bf3/t+aRp3zBE7EAAAAASUVORK5CYII='><meta name='description' content='grrd s WordClock is a web WordClock and a user interface for the Wemos Mini D1 Lite Clock'><meta name='viewport' content='width=device-width,initial-scale=1'><meta name='theme-color' content='#444'><meta name='apple-mobile-web-app-title' content='WordClock'><link id='iphone' rel='apple-touch-icon'><meta name='apple-mobile-web-app-capable' content='yes'><meta name='apple-mobile-web-app-status-bar-style' content='black'>"));
              // Styles
              client.println(F("<style>:root{--main-color:#878ade}html{height:100%;user-select:none}body{background:linear-gradient(#444,#222);min-width:100vw;margin:0;position:fixed;overflow:hidden;font-family:Arial,sans-serif;font-size:large;color:#fff;text-shadow:1px 1px 2px #000;height:100%}.page{width:100vw;position:fixed;top:0;left:0;right:0;bottom:0;background:linear-gradient(#444,#222)}#clock,#control,.title{font:6px sans-serif;fill:#555;text-shadow:none;text-anchor:middle;width:100vmin;margin:auto;display:block}@media (orientation:landscape){#control{width:100%;max-width:60vh}}@media (orientation:portrait){#control{width:100%}}.title,.w100{width:100%}.H0 .H0,.H1 .H1,.H10 .H10,.H11 .H11,.H2 .H2,.H3 .H3,.H4 .H4,.H5 .H5,.H6 .H6,.H7 .H7,.H8 .H8,.H9 .H9,.M1 .M1,.M10 .M10,.M15 .M15,.M2 .M1,.M2 .M2,.M20 .M20,.M3 .M1,.M3 .M2,.M3 .M3,.M30 .M30,.M4 .M1,.M4 .M2,.M4 .M3,.M4 .M4,.M5 .M5,.MA .MA,.MV .MV,.g{fill:var(--main-color);text-shadow:0 0 10px var(--main-color)}.off .g:not(.colorButton){fill:#555;text-shadow:none}.d .H0 .H0,.d .H1 .H1,.d .H10 .H10,.d .H11 .H11,.d .H2 .H2,.d .H3 .H3,.d .H4 .H4,.d .H5 .H5,.d .H6 .H6,.d .H7 .H7,.d .H8 .H8,.d .H9 .H9,.d .M1 .M1,.d .M10 .M10,.d .M15 .M15,.d .M2 .M1,.d .M2 .M2,.d .M20 .M20,.d .M3 .M1,.d .M3 .M2,.d .M3 .M3,.d .M30 .M30,.d .M4 .M1,.d .M4 .M2,.d .M4 .M3,.d .M4 .M4,.d .M5 .M5,.d .MA .MA,.d .MV .MV,.d .g{filter:brightness(70%)}a:link{color:var(--main-color)}a:visited{color:var(--main-color);filter:brightness(85%)}a:focus,a:hover{color:var(--main-color);filter:brightness(125%)}a:active{color:var(--main-color);filter:brightness(125%)}#exitMastermind,#exitSnake,#exitWordGuessr,#settings,#settingsClose{position:absolute;right:4vmin;bottom:4vmin}#power{position:absolute;left:4vmin;bottom:4vmin}#scoreMastermind,#scoreSnake,#scoreWordGuessr{position:absolute;left:4vmin;bottom:4vmin;display:flex;align-items:center}.snakeButton,.svgButton,.svgMsg{width:4.5vmin;height:4.5vmin;min-width:30px;min-height:30px;stroke:#555;stroke-linejoin:round;stroke-linecap:round;stroke-width:6;fill:none;z-index:1000}circle{pointer-events:none}input[type=text]{width:calc(100% - 4.5vmin - 40px);border:2px solid #555;border-radius:5px;background-color:transparent;color:#fff;padding:10px;font-size:larger}input[type=text]:focus{border:2px solid #fff;outline:0}input[type=text].error{border:2px solid #f70562}input[type=text].ok{border:2px solid #059c7d}.snakeButton.g,.snakeButton:hover,.svgButton.g,.svgButton:hover{stroke:#fff;text-shadow:0 0 10px #fff;cursor:pointer}.snakeButton{stroke-width:1.4;fill:#333;fill-opacity:0.01}#pageMastermind,#pageSettings,#pageSnake,#pageWordGuessr{transform:translateX(100vw);visibility:hidden;opacity:0}.pageContent{display:block;position:absolute;overflow:auto;top:0;left:0;right:0;margin:0 auto 0 auto;width:600px;max-width:calc(100vw - 40px);height:100%}.content,.content>span,.pageBody{display:flex;flex-direction:row;justify-content:space-between;margin-bottom:20px;align-items:center;flex-wrap:wrap}.pageFooter{margin-top:60px}#color{-webkit-appearance:none;-moz-appearance:none;appearance:none;background-color:transparent;width:4.5vmin;height:4.5vmin;min-width:30px;min-height:30px;border:none;cursor:pointer}#color::-webkit-color-swatch{border-radius:50%;border:.45vmin solid #555}#color::-moz-color-swatch{border-radius:50%;border:.45vmin solid #555}#color::-webkit-color-swatch:hover{border:.45vmin solid #fff}#color::-moz-color-swatch:hover{border:.45vmin solid #fff}.hide{display:none}[data-num='1']{fill:#fc034e}[data-num='2']{fill:#fc6f03}[data-num='3']{fill:#fcce03}[data-num='4']{fill:#18fc03}[data-num='5']{fill:#0384fc}[data-num='6']{fill:#f803fc}.swipe-in{animation-name:swipe-in;animation-fill-mode:forwards;animation-duration:.7s}@keyframes swipe-in{0%{transform:translateX(100vw);visibility:hidden;opacity:0}1%{transform:translateX(100vw);visibility:visible;opacity:1}100%{transform:translateX(0);visibility:visible;opacity:1}}.swipe-out{animation-name:swipe-out;animation-fill-mode:forwards;animation-duration:.7s}@keyframes swipe-out{0%{transform:translateX(0);visibility:visible;opacity:1}99%{transform:translateX(-100vw);visibility:visible;opacity:1}100%{transform:translateX(-100vw);visibility:hidden;opacity:0}}.swipe-in-left{animation-name:swipe-in-left;animation-fill-mode:forwards;animation-duration:.7s}@keyframes swipe-in-left{0%{transform:translateX(0);visibility:visible;opacity:1}99%{transform:translateX(100vw);visibility:visible;opacity:1}100%{transform:translateX(100vw);visibility:hidden;opacity:0}}.swipe-out-right{animation-name:swipe-out-right;animation-fill-mode:forwards;animation-duration:.7s}@keyframes swipe-out-right{0%{transform:translateX(-100vw);visibility:hidden;opacity:0}1%{transform:translateX(-100vw);visibility:visible;opacity:1}100%{transform:translateX(0);visibility:visible;opacity:1}}.slider{-webkit-appearance:none;width:100%;height:4px;border-radius:2px;background:0 0;margin:10px 0;direction:rtl;border:solid calc(2px + .2vmin) #555}.slider::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:3vmin;height:3vmin;min-width:20px;min-height:20px;border-radius:50%;background:var(--main-color);cursor:pointer;outline:solid .45vmin #555}.slider::-webkit-slider-thumb:hover{outline:solid .45vmin #fff}.slider::-moz-range-thumb{width:3vmin;height:3vmin;min-width:20px;min-height:20px;border-radius:50%;background:var(--main-color);cursor:pointer;outline:solid .45vmin #555}.slider::-moz-range-thumb:hover{outline:solid .45vmin #fff}</style>"));
              // Body
              client.println(F("</head><body><div id='pageClock' class='page'><svg id='clock' viewBox='0 0 115 110' preserveAspectRatio='xMidYMid slice' role='img'><text x='07' y='10' class='g'>E</text><text x='17' y='10' class='g'>S</text><text x='27' y='10'>D</text><text x='37' y='10' class='g'>I</text><text x='47' y='10' class='g'>S</text><text x='57' y='10' class='g'>C</text><text x='67' y='10' class='g'>H</text><text x='77' y='10'>W</text><text x='87' y='10' class='M5'>F</text><text x='97' y='10' class='M5'>Ü</text><text x='107' y='10' class='M5'>F</text><text x='07' y='20' class='M15'>V</text><text x='17' y='20' class='M15'>I</text><text x='27' y='20' class='M15'>E</text><text x='37' y='20' class='M15'>R</text><text x='47' y='20' class='M15'>T</text><text x='57' y='20' class='M15'>U</text><text x='67' y='20'>T</text><text x='77' y='20' class='M10'>Z</text><text x='87' y='20' class='M10'>Ä</text><text x='97' y='20' class='M10'>Ä</text><text x='107' y='20'>Y</text><text x='07' y='30' class='M20'>Z</text><text x='17' y='30' class='M20'>W</text><text x='27' y='30' class='M20'>Ä</text><text x='37' y='30' class='M20'>N</text><text x='47' y='30' class='M20'>Z</text><text x='57' y='30' class='M20'>G</text><text x='67' y='30'>Q</text><text x='77' y='30'>D</text><text x='87' y='30' class='MV'>V</text><text x='97' y='30' class='MV'>O</text><text x='107' y='30' class='MV'>R</text><text x='07' y='40'>K</text><text x='17' y='40' class='MA'>A</text><text x='27' y='40' class='MA'>B</text><text x='37' y='40'>D</text><text x='47' y='40' class='M30'>H</text><text x='57' y='40' class='M30'>A</text><text x='67' y='40' class='M30'>U</text><text x='77' y='40' class='M30'>B</text><text x='87' y='40' class='M30'>I</text><text x='97' y='40'>T</text><text x='107' y='40'>Z</text><text x='07' y='50' class='H1'>E</text><text x='17' y='50' class='H1'>I</text><text x='27' y='50' class='H1'>S</text><text x='37' y='50'>Q</text><text x='47' y='50' class='H2'>Z</text><text x='57' y='50' class='H2'>W</text><text x='67' y='50' class='H2'>Ö</text><text x='77' y='50' class='H2'>I</text><text x='87' y='50' class='H3'>D</text><text x='97' y='50' class='H3'>R</text><text x='107' y='50' class='H3'>Ü</text><text x='07' y='60'>Z</text><text x='17' y='60' class='H4'>V</text><text x='27' y='60' class='H4'>I</text><text x='37' y='60' class='H4'>E</text><text x='47' y='60' class='H4'>R</text><text x='57' y='60' class='H4'>I</text><text x='67' y='60' class='H5'>F</text><text x='77' y='60' class='H5'>Ü</text><text x='87' y='60' class='H5'>F</text><text x='97' y='60' class='H5'>I</text><text x='107' y='60'>T</text><text x='07' y='70'>G</text><text x='17' y='70'>M</text><text x='27' y='70' class='H6'>S</text><text x='37' y='70' class='H6'>Ä</text><text x='47' y='70' class='H6'>C</text><text x='57' y='70' class='H6'>H</text><text x='67' y='70' class='H6 H7'>S</text><text x='77' y='70' class='H6 H7'>I</text><text x='87' y='70' class='H7'>B</text><text x='97' y='70' class='H7'>N</text><text x='107' y='70' class='H7'>I</text><text x='07' y='80' class='H8'>A</text><text x='17' y='80' class='H8'>C</text><text x='27' y='80' class='H8'>H</text><text x='37' y='80' class='H8'>T</text><text x='47' y='80' class='H8'>I</text><text x='57' y='80' class='H9'>N</text><text x='67' y='80' class='H9'>Ü</text><text x='77' y='80' class='H9'>N</text><text x='87' y='80' class='H9'>I</text><text x='97' y='80'>O</text><text x='107' y='80'>F</text><text x='07' y='90'>C</text><text x='17' y='90'>D</text><text x='27' y='90' class='H10'>Z</text><text x='37' y='90' class='H10'>Ä</text><text x='47' y='90' class='H10'>N</text><text x='57' y='90' class='H10'>I</text><text x='67' y='90'>X</text><text x='77' y='90' class='H11'>E</text><text x='87' y='90' class='H11'>U</text><text x='97' y='90' class='H11'>F</text><text x='107' y='90' class='H11'>I</text><text x='07' y='100'>O</text><text x='17' y='100'>K</text><text x='27' y='100'>G</text><text x='37' y='100' class='H0'>Z</text><text x='47' y='100' class='H0'>W</text><text x='57' y='100' class='H0'>Ö</text><text x='67' y='100' class='H0'>U</text><text x='77' y='100' class='H0'>F</text><text x='87' y='100' class='H0'>I</text><text x='97' y='100'>L</text><text x='107' y='100'>X</text><text x='07' y='110'>L</text><text x='17' y='110'>Y</text><text x='27' y='110'>B</text><text x='37' y='112.5' class='M1' font-size='8'>°</text><text x='47' y='112.5' class='M2' font-size='8'>°</text><text x='57' y='110'>P</text><text x='67' y='112.5' class='M3' font-size='8'>°</text><text x='77' y='112.5' class='M4' font-size='8'>°</text><text x='87' y='110'>M</text><text x='97' y='110'>K</text><g stroke='#555' fill='none' stroke-width='0.7'><path d='M 106 109.8 Q 106 106.4 109.4 106.4'/><path d='M 107.2 109.8 Q 107.2 107.8 109.4 107.8'/></g><circle cx='108.8' cy='109.5' r='0.6' fill='#555'/></svg> <svg id='power' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 74 74'><line x1='37' y1='15' x2='37' y2='27'/><circle cx='37' cy='37' r='33'/><path d='M 48 22 A 18 18 0 1 1 26 22'/></svg> <svg id='settings' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 74 74'><path d='M30 3 A 37 37 0 0 1 44 3 L 44 13 A 25 25 0 0 1 54.5 20 L 63 14 A 37 37 0 0 1 70 25.5 L 61 31 A 25 25 0 0 1 61 42.5 L 70 48.5 A 37 37 0 0 1 63 60 L 54.5 54 A 25 25 0 0 1 44 61 L 44 71 A 37 37 0 0 1 30 71 L 30 61 A 25 25 0 0 1 19.5 54 L 11 60 A 37 37 0 0 1 4 48.5 L 13 42.5 A 25 25 0 0 1 13 31 L 4 25.5 A 37 37 0 0 1 11 14 L 19.5 20 A 25 25 0 0 1 30 13 Z'/><circle cx='37' cy='37' r='12'/></svg></div><div id='pageSettings' class='page'><div class='pageContent'><div class='pageHead'><svg class='title' viewBox='5 0 105 25' preserveAspectRatio='xMidYMid slice' role='img'></svg></div><div class='pageBody'><label for='color'>Weli Farb wosch?</label> <input type='color' id='color' name='head' value='#ffffff'></div><div class='pageBody'><label>Cha mi nid entscheide. Chli vo auem.</label> <svg id='rainbowMode' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 70 70'><path transform='scale(0.9) translate(5,5)' d='M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z'/><path class='hide' transform='scale(0.85) translate(5,5)' d='M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z'/></svg></div><div class='pageBody'><label class='w100' for='speed'>Wie schnäu?</label> <input type='range' id='speed' class='slider' min='50' max='2000'> <label>gmüetlech</label> <label>jufle</label></div><div class='pageBody'><label>Ir Nacht chli weniger häu.</label> <svg id='darkMode' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 70 70'><path class='hide' transform='scale(0.9) translate(5,5)' d='M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z'/><path transform='scale(0.85) translate(5,5)' d='M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z'/></svg></div><div class='pageBody'><label>I ha ke Angst vor Gspängster.</label> <svg id='ghostMode' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 70 70'><path class='hide' transform='scale(0.9) translate(5,5)' d='M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z'/><path transform='scale(0.85) translate(5,5)' d='M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z'/></svg></div><div id='snakeBody' class='pageBody'><label>Schnäu e Rundi Snake spile.</label> <svg id='playSnake' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='-2 -1 12 16'><path d='M2 2 L9 7 L2 12 Z' stroke-width='1.4'/></svg></div><div id='mastermindBody' class='pageBody'><label>Oder hurti es Mastermind.</label> <svg id='playMastermind' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='-2 -1 12 16'><path d='M2 2 L9 7 L2 12 Z' stroke-width='1.4'/></svg></div><div id='wordGuessrBody' class='pageBody'><label>Es paar Wörtli errate.</label> <svg id='playWordGuessr' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='-2 -1 12 16'><path d='M2 2 L9 7 L2 12 Z' stroke-width='1.4'/></svg></div><div class='pageFooter'><p class='content'>Handgmachti Software us Bärn</p><p class='content'><span>Gérard&nbsp;Tyedmers</span> <span><svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' viewBox='0 -5 160 170' stroke='#fff' fill='none' stroke-width='10'><circle cx='80' cy='80' r='70'/><path d='M27 32c7 20 93 43 121 28M13 60c-3 30 117 60 135 35M16 106c16 19 84 39 112 24M100 13C34 3 10 130 65 148M100 13C70 33 45 118 65 148M100 13c13 22 5 112-35 135M100 13c60 35 20 147-35 135'/></svg>&nbsp; <a href='https://grrd.ch'>grrd.ch</a> </span><span><svg xmlns='http://www.w3.org/2000/svg' width='30px' height='24px' viewBox='0 0 222 179' stroke='#fff' fill='none' stroke-width='10' stroke-linecap='round'><g transform='translate(-10,10) rotate(-6)'><rect x='15' y='25' rx='10' ry='10' width='192' height='129'/><path d='M15 40 C131 125, 91 125, 207 40'/><line x1='15' y1='134' x2='77' y2='90'/><line x1='207' y1='134' x2='145' y2='90'/></g></svg>&nbsp; <a href='mailto:grrd@gmx.net'>grrd@gmx.net</a></span></p></div></div><svg id='settingsClose' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 70 70'><path transform='scale(0.85) translate(5,5)' d='M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z'/></svg></div><div id='pageSnake' class='page'><div class='pageContent'><div class='pageHead'><svg class='title' viewBox='5 0 105 25' preserveAspectRatio='xMidYMid slice' role='img'></svg></div><svg id='control' viewBox='0 0 115 110' preserveAspectRatio='xMidYMid slice' role='img'><path data-num='1' class='snakeButton' transform='scale(3.8) translate(8,10) rotate(270)' d='M2 2 L9 7 L2 12 Z'/><path data-num='2' class='snakeButton' transform='scale(3.8) translate(20,7.5)' d='M2 2 L9 7 L2 12 Z'/><path data-num='3' class='snakeButton' transform='scale(3.8) translate(22,19) rotate(90)' d='M2 2 L9 7 L2 12 Z'/><path data-num='4' class='snakeButton' transform='scale(3.8) translate(10,21.5) rotate(180)' d='M2 2 L9 7 L2 12 Z'/></svg></div><svg id='exitSnake' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 70 70'><path transform='scale(0.9) translate(5,5)' d='M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z'/></svg> <span id='scoreSnake'></span></div><div id='pageMastermind' class='page'><div class='pageContent'><div class='pageHead'><svg class='title' viewBox='5 0 105 25' preserveAspectRatio='xMidYMid slice' role='img'></svg></div><label class='content'>Hie chasch d Farb useläse:</label> <span class='content'><svg xmlns='http://www.w3.org/2000/svg' class='svgButton colorButton' data-num='1' viewBox='0 0 70 70'><circle cx='35' cy='35' r='25'/></svg> <svg xmlns='http://www.w3.org/2000/svg' class='svgButton colorButton' data-num='2' viewBox='0 0 70 70'><circle cx='35' cy='35' r='25'/></svg> <svg xmlns='http://www.w3.org/2000/svg' class='svgButton colorButton' data-num='3' viewBox='0 0 70 70'><circle cx='35' cy='35' r='25'/></svg> <svg xmlns='http://www.w3.org/2000/svg' class='svgButton colorButton' data-num='4' viewBox='0 0 70 70'><circle cx='35' cy='35' r='25'/></svg> <svg xmlns='http://www.w3.org/2000/svg' class='svgButton colorButton' data-num='5' viewBox='0 0 70 70'><circle cx='35' cy='35' r='25'/></svg> <svg xmlns='http://www.w3.org/2000/svg' class='svgButton colorButton' data-num='6' viewBox='0 0 70 70'><circle cx='35' cy='35' r='25'/></svg> </span><label class='content'>Hie muesch di Versuech iigäh:</label> <span class='content'><svg xmlns='http://www.w3.org/2000/svg' class='svgButton codeButton' viewBox='0 0 70 70'><circle cx='35' cy='35' r='25'/></svg> <svg xmlns='http://www.w3.org/2000/svg' class='svgButton codeButton' viewBox='0 0 70 70'><circle cx='35' cy='35' r='25'/></svg> <svg xmlns='http://www.w3.org/2000/svg' class='svgButton codeButton' viewBox='0 0 70 70'><circle cx='35' cy='35' r='25'/></svg> <svg xmlns='http://www.w3.org/2000/svg' class='svgButton codeButton' viewBox='0 0 70 70'><circle cx='35' cy='35' r='25'/></svg> <svg id='sendMastermind' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 70 70'><path transform='scale(0.85) translate(5,5)' d='M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z'/></svg></span></div><svg id='exitMastermind' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 70 70'><path transform='scale(0.9) translate(5,5)' d='M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z'/></svg> <span id='scoreMastermind'></span></div><div id='pageWordGuessr' class='page'><div class='pageContent'><div class='pageHead'><svg class='title' viewBox='4 0 106 25' preserveAspectRatio='xMidYMid slice' role='img'></svg></div><label for='wordInput' class='content'><br>Weles isch ds gsuechte Wort:</label> <span class='content'><input type='text' id='wordInput' maxlength='20' oninput='this.value = this.value.toUpperCase().replace(/[^A-ZÄÖÜ]/g, &#39;&#39;)' spellcheck='false' autocomplete='off'> <svg id='sendWordGuessr' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 70 70'><path transform='scale(0.85) translate(5,5)' d='M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z'/></svg></span></div><svg id='exitWordGuessr' class='svgButton' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 70 70'><path transform='scale(0.9) translate(5,5)' d='M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z'/></svg> <span id='scoreWordGuessr'></span></div>"));
              // Script
              client.println(F("<script>!function(){'use strict';let e,t,s,n=1,a=1,i=1,c=0,r=255,o=0,d=0,l=0,u=0,g='1',f=0,w=0,m=0;function L(e){return document.getElementById(e)}function h(e){return localStorage.getItem(e)}function p(e,t){return localStorage.setItem(e,t)}const v=L('clock'),M=L('pageClock'),k=L('pageSettings'),E=L('pageSnake'),b=L('pageMastermind'),y=L('pageWordGuessr'),S=L('color'),A=L('speed'),_=L('wordInput'),T=L('rainbowMode'),x=L('ghostMode'),R=L('darkMode'),G=document.getElementsByTagName('body')[0],B=document.getElementsByClassName('codeButton'),I=document.getElementsByClassName('colorButton');function H(){c&&(r&&!d?(r-=1,o+=1):o?(o-=1,d+=1):(d-=1,r+=1),W('rgb('+r+', '+o+', '+d+')'),setTimeout(H,A.value/10))}function C(e){n=e,n?G.classList.remove('off'):G.classList.add('off'),s=-1}function N(e,t){document.activeElement.blur(),e.classList.remove('swipe-out-right'),t.classList.remove('swipe-in-left'),e.classList.add('swipe-out'),t.classList.add('swipe-in')}function D(e,t){e.classList.remove('swipe-out'),t.classList.remove('swipe-in'),e.classList.add('swipe-out-right'),t.classList.add('swipe-in-left')}function W(e){document.documentElement.style.setProperty('--main-color',e)}function O(e){e!==c&&(T.children[0].classList.toggle('hide'),T.children[1].classList.toggle('hide')),c=e,c?H():W(S.value)}function q(e){e!==i&&(x.children[0].classList.toggle('hide'),x.children[1].classList.toggle('hide')),i=e}function j(e){e!==a&&(R.children[0].classList.toggle('hide'),R.children[1].classList.toggle('hide')),a=e}function V(){let e=parseInt(S.value.substring(1,3),16),t=parseInt(S.value.substring(3,5),16),s=parseInt(S.value.substring(5,7),16);if(p('wc_color',S.value),p('wc_rainbow',c),p('wc_dark',a),p('wc_ghost',i),p('wc_speed',A.value.toString()),window.location.href.includes('192.168.')||window.location.href.includes('.local')){let r=new XMLHttpRequest;r.open('GET','/update_params?red='+e+'&green='+t+'&blue='+s+'&rainbow='+c+'&darkmode='+a+'&speed='+A.value+'&power='+n+'&ghost='+i,!0),r.send()}}function X(e){let t=new XMLHttpRequest;t.onreadystatechange=function(){4===this.readyState&&200===this.status&&(l=10*(parseInt(t.responseText)-3),l>u&&(u=l,p('wc_score',u)),L('scoreSnake').innerHTML='Score: '+l+' / High-Score : '+u)},t.open('GET','snake?dir='+e,!0),t.send()}function J(e){let t='';if(1===e)t='mastermind?c4=0',U(),z();else if(2===e)t='mastermind?c4=7',U();else{if(document.querySelectorAll('[data-num=\"1\"], [data-num=\"2\"], [data-num=\"3\"], [data-num=\"4\"], [data-num=\"5\"], [data-num=\"6\"]').length<14)return void z('Muesch zersch aues uswähle.');t='mastermind?c1='+B[0].getAttribute('data-num')+'&c2='+B[1].getAttribute('data-num')+'&c3='+B[2].getAttribute('data-num')+'&c4='+B[3].getAttribute('data-num'),U()}let s=new XMLHttpRequest;s.onreadystatechange=function(){if(4===this.readyState&&200===this.status){let e=JSON.parse(s.responseText);f=e.place,w=e.try,4===f?z('Bravo! I '+w+' Mau usegfunde.'):11===w?z('Schad, jetz hesch verlore.'):z()}},s.open('GET',t,!0),s.send()}function U(){Array.from(B).forEach((function(e){e.setAttribute('data-num','')}))}function z(e){L('scoreMastermind').innerHTML=e||'<svg xmlns=\"http://www.w3.org/2000/svg\" class=\"svgMsg\" viewBox=\"0 0 70 70\"> <circle cx=\"35\" cy=\"35\" r=\"25\" fill=\"white\"/></svg>&nbsp;am richtige Ort&nbsp;<svg xmlns=\"http://www.w3.org/2000/svg\" class=\"svgMsg\" viewBox=\"0 0 70 70\"> <circle cx=\"35\" cy=\"35\" r=\"25\" fill=\"cornflowerblue\"/></svg>&nbsp;di richtigi Farb'}function K(e){let t;t='1'===e?'wordguessr?new':'2'===e?'wordguessr?exit':'wordguessr?word='+_.value;let s=new XMLHttpRequest;s.onreadystatechange=function(){if(4===this.readyState&&200===this.status){let e=JSON.parse(s.responseText);0===e.score?(_.classList.add('error'),setTimeout((function(){_.classList.remove('error'),_.value=''}),100)):1===e.score&&(m+=e.score,L('scoreWordGuessr').innerHTML=m+' hesch usegfunde.',_.classList.add('ok'),setTimeout((function(){_.classList.remove('ok'),_.value=''}),100))}},s.open('GET',t,!0),s.send()}setInterval((function(){e=new Date,a&&(e.getHours()>=22||e.getHours()<7)?G.classList.add('d'):G.classList.remove('d'),s!==e.getMinutes()&&(s=e.getMinutes(),v.classList.remove(...v.classList),0!==n&&(s>=55?v.classList.add('M5','MV'):s>=50?v.classList.add('M10','MV'):s>=45?v.classList.add('M15','MV'):s>=40?v.classList.add('M20','MV'):s>=35?v.classList.add('M5','MA','M30'):s>=30?v.classList.add('M30'):s>=25?v.classList.add('M5','MV','M30'):s>=20?v.classList.add('M20','MA'):s>=15?v.classList.add('M15','MA'):s>=10?v.classList.add('M10','MA'):s>=5&&v.classList.add('M5','MA'),t=e.getHours(),s>=25&&(t+=1),t%=12,v.classList.add('H'+t.toString()),v.classList.add('M'+(s%5).toString())))}),100),L('power').addEventListener('click',(function(){C(1-n),V()})),L('settings').addEventListener('click',(function(){N(M,k)})),L('settingsClose').addEventListener('click',(function(){D(M,k),k.classList.remove('swipe-out-right'),V()})),L('playSnake').addEventListener('click',(function(){N(k,E),X(5)})),L('exitSnake').addEventListener('click',(function(){D(k,E),X(6)})),L('playMastermind').addEventListener('click',(function(){N(k,b),J(1)})),L('exitMastermind').addEventListener('click',(function(){D(k,b),J(2)})),L('sendMastermind').addEventListener('click',J),L('playWordGuessr').addEventListener('click',(function(){N(k,y),K('1'),m=0,L('scoreWordGuessr').innerHTML=''})),L('exitWordGuessr').addEventListener('click',(function(){D(k,y),K('2')})),L('sendWordGuessr').addEventListener('click',K),Array.from(document.getElementsByClassName('snakeButton')).forEach((function(e){e.addEventListener('click',(function(e){X(e.target.getAttribute('data-num'))}))})),Array.from(I).forEach((function(e){e.addEventListener('click',(function(e){Array.from(I).forEach((function(e){e.classList.remove('g')})),e.target.classList.add('g'),g=e.target.getAttribute('data-num')}))})),Array.from(B).forEach((function(e){e.addEventListener('click',(function(e){e.target.setAttribute('data-num',g),z()}))})),document.onkeydown=function(e){let t=0;switch(e.key){case'ArrowUp':t=1;break;case'ArrowRight':t=2;break;case'ArrowDown':t=3;break;case'ArrowLeft':t=4;break;case'Enter':y.classList.contains('swipe-in')&&K()}t&&E.classList.contains('swipe-in')&&(X(t),L('control').children[t-1].classList.add('g'),setTimeout((function(){L('control').children[t-1].classList.remove('g')}),200))},S.addEventListener('change',(e=>{W(S.value)}),!1),T.addEventListener('click',(e=>{O(1-c)})),x.addEventListener('click',(e=>{q(1-i)})),R.addEventListener('click',(e=>{j(1-a)})),h('wc_color')&&(S.value=h('wc_color'),W(S.value)),h('wc_rainbow')&&O(parseInt(h('wc_rainbow'))),h('wc_ghost')&&q(parseInt(h('wc_ghost'))),h('wc_dark')&&j(parseInt(h('wc_dark'))),h('wc_speed')&&(A.value=parseInt(h('wc_speed'))),h('wc_score')&&(u=h('wc_score')),L('iphone').href=L('icon').href;const F=['ewfGRRDcSajnWORDuCLOCK','ewfGRRDcSajmSNAKExlbdk','ewfGRRDcSajMASTERMINDk','ewfGRRDcSajWORDbGUESSR'];if(Array.from(document.getElementsByClassName('title')).forEach((function(e,t){for(let s=0;s<22;s++){const n=document.createElementNS('http://www.w3.org/2000/svg','text');let a=F[t].substring(s,s+1),i=a.toUpperCase();n.setAttribute('x',s%11*10+7),n.setAttribute('y',10*Math.ceil((s+1)/11)),n.textContent=i,a===i&&n.setAttribute('class','g'),e.appendChild(n)}})),window.location.href.includes('192.168.')||window.location.href.includes('.local')){let e=new XMLHttpRequest;e.onreadystatechange=function(){if(4===this.readyState&&200===this.status){let a=JSON.parse(e.responseText);a.rainbow||(S.value=(t=a.red,s=a.green,n=a.blue,'#'+(1<<24|t<<16|s<<8|n).toString(16).slice(1))),W(S.value),j(a.darkmode),O(a.rainbow),q(a.ghost),C(a.power),A.value=a.speed}var t,s,n},e.open('GET','get_params',!0),e.send()}else L('snakeBody').classList.add('hide'),L('mastermindBody').classList.add('hide'),L('wordGuessrBody').classList.add('hide')}();</script></body></html>"));
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
  }
  MDNS.update();

  // sleep and return when power off
  if (power == 0) {
    delay(500);
    return;
  }

  // display ghost if enabled
  if (ghost == 1 && ghostHour == wordClockHour && ghostMinute == wordClockMinute) {
    if (!inGhost) {
      inGhost = true;
      // loop 11 Zeilen
      for (int j = 11; j >= 0; j--) {
        blank();
        // Schleife durch das Array
        for (int i = 0; i < 121; i++) {
          int ghostPixel = WordGhost[i];
          // Wenn der Wert -1 erreicht wird, die Schleife beenden
          if (WordGhost[i] == -1) {
            break;
          }
          // pixel um anzahl Zeilen nach unten verschieben
          ghostPixel = down(ghostPixel,j);
          pixels.setPixelColor(ghostPixel, dimColor(White, (11 - (float)j) / 11));
        }
        pixels.show();
        delay(200);
      }
    } else {
      // ghost eyes
      ghostStep = ghostStep + ghostChange;
      if (ghostStep == 100) {
        ghostChange = -1;
      } else if (ghostStep == 0) {
        ghostChange = 1;
      }
      // Calculate the RGB values for the current step
      int red   = (120 * ghostStep) / 100;    // Red value from 0 to 100
      int blue  = (6 * ghostStep) / 100;      // Blue value from 0 to 5
    
      lightup(WordGhostEyes, pixels.Color(red, 0, blue));
      pixels.show();
      delay(10);  
      getLocalTime();
    }
    
    return;
  }
  // hide ghost
  if (inGhost && ghostMinute != wordClockMinute) {
    // hide ghost
    inGhost = false;
    // loop 11 Zeilen
      for (int j = 0; j < 11; j++) {
        blank();
        // Schleife durch das Array
        for (int i = 0; i < 121; i++) {
          int ghostPixel = WordGhost[i];
          // Wenn der Wert -1 erreicht wird, die Schleife beenden
          if (WordGhost[i] == -1) {
            break;
          }
          // pixel um anzahl Zeilen nach unten verschieben
          ghostPixel = down(ghostPixel,j);
          pixels.setPixelColor(ghostPixel, dimColor(White, (11 - (float)j) / 11));
        }
        pixels.show();
        delay(200);
      }
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
        if (floor(snakeNext / 11) != snake[0] / 11 || snakeNext == -1) {
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
  } else if (inWordGuessr) {
    if (wordGuessrAlert > 0 && wordGuessrAlert < millis()) {
      // nach Alert (richtig/falsch) wieder auf normale Anzeige wechseln
      blank();
      lightup(wordGuessrActiveWordIndex, foregroundColor);
      for (int i = 0; i < wordGuessrHint; i++) {
          pixels.setPixelColor(wordGuessrActiveWordIndex[i], Green);
        }
      pixels.show();
      wordGuessrAlert = 0;
    } else if (millis() > wordGuessrStart + (wordGuessrHint + 1) * 30000) {
      // nach 30 Sekunden einen weiteren Buchstaben als Hint grün färben
      pixels.setPixelColor(wordGuessrActiveWordIndex[wordGuessrHint], Green);
      pixels.show();
      wordGuessrHint++;
      // falls alle Buchstaben schon grün sind, zum nächsten Wort wechseln
      if (wordGuessrActiveWordIndex[wordGuessrHint] == -1) {
        wordGuessrNewGuess();
        wordGuessrAlert = millis() + 100;
      }
    }
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

  if (timeStatus() != timeNotSet && !inSnake && !inMastermind && !inWordGuessr) {
    if (lastMinuteWordClock != wordClockMinute) { //update the display only if time has changed
      getLocalTime();
      displayTime();
      lastMinuteWordClock = wordClockMinute;
    } else {
      displayWifiStatus();
      getLocalTime();
    }
  }
}
