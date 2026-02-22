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
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>        // v2.0.17
#include <WiFiUdp.h>
#include <WebSocketsServer.h>   // v2.7.1
#include <TimeLib.h>            // v1.6.1
#include <Timezone.h>           // v1.2.6
#include <Adafruit_NeoPixel.h>  // v1.15.2
#include <pgmspace.h>
#include <EEPROM.h>
#include "web_interface.h"

// set name for access-point and mdns-server
const char* version = "wordclock";

// Set web server port number to 80, WebSocketsServer to 81
WiFiServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the web parameters
const uint8_t numPixels = 121;
uint8_t rgbRed = 255;
uint8_t rgbGreen = 255;
uint8_t rgbBlue = 255;
uint8_t power = 1;
uint8_t darkMode = 1;
uint8_t ghost = 1;
uint8_t effect = 0; // 0 = none, 1 = colorWheel, 2 = rainbow, 3 = matrix, 4 = pulse, 5 = typewriter
int effectSpeed = 200;
int effectWait = 200;
bool lastTouchStage = false;
uint16_t frame = 0;
// Matrix-Drops
struct Drop {
  float y; // aktuelle Position (kann zwischen den Zeilen liegen)
  float speed; // Geschwindigkeit (Zeilen pro Frame)
  uint8_t length; // Abdunklungslänge
  uint8_t maxBrightness; // Maximale Helligkeit (0..255)
  bool active; // Ob dieser Drop aktiv ist
};
Drop drops[11][2]; // 11 Spalten, bis zu 2 Drops pro Spalte
uint16_t dropCooldown[11]; // Cooldown pro Spalte bis zum nächsten neuen Drop

// EEPROM addresses for persistent variables
const uint8_t eepromAddrRed = 0;
const uint8_t eepromAddrGreen = 1;
const uint8_t eepromAddrBlue = 2;
const uint8_t eepromAddrDarkMode = 3;
const uint8_t eepromAddrGhost = 4;
const uint8_t eepromAddrEffect = 5;
const uint8_t eepromAddrEffectSpeedLow = 6;   // effectSpeed is int (2 bytes)
const uint8_t eepromAddrEffectSpeedHigh = 7;
const uint8_t eepromAddrTetrisHigh = 8;
const uint8_t eepromAddrSnakeHigh = 9;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

uint8_t wordClockMinute = 62;
uint8_t wordClockHour = 0;
uint8_t lastMinuteWordClock = 61;

const uint8_t NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
static const char ntpServerName[] = "0.ch.pool.ntp.org";

const uint8_t timeZone = 0;     // Central European Time

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
static uint32_t Cornflower = Adafruit_NeoPixel::Color(8, 7, 16);
static uint32_t Red = Adafruit_NeoPixel::Color(100, 0, 5);
static uint32_t Orange = Adafruit_NeoPixel::Color(100, 25, 0);
static uint32_t Yellow = Adafruit_NeoPixel::Color(100, 90, 0);
static uint32_t Green = Adafruit_NeoPixel::Color(10, 95, 0);
static uint32_t Blue = Adafruit_NeoPixel::Color(0, 20, 85);
static uint32_t Purple = Adafruit_NeoPixel::Color(85, 0, 100);
static uint32_t Cyan = Adafruit_NeoPixel::Color(00, 100, 100);

static uint32_t GameColors[] = {Red, Orange, Yellow, Green, Blue, Purple, Cyan};

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

static int8_t WordEs[] = {0, 1, -1};
static int8_t WordIst[] = {3, 4, 5, 6, -1};
static int8_t WordHalb[] = {39, 38, 37, 36, 35, -1};

static int8_t WordFix[] = {77, 98, 99, -1};
static int8_t WordWifi[] = {120, -1};
static int8_t WordNach[] = {42, 41, -1};
static int8_t WordVor[] = {30, 31, 32, -1};

static int8_t SymbolWifi[] = {120, -1};

static int8_t *WordURL[] = {WordFix, WordWifi};

// Stunde
static int8_t WordStundeEins[] = {44, 45, 46, -1};                     // EIS
static int8_t WordStundeZwei[] = {48, 49, 50, 51, -1};                 // ZWÖI
static int8_t WordStundeDrei[] = {52, 53, 54, -1};                     // DRÜ
static int8_t WordStundeVier[] = {64, 63, 62, 61, 60, -1};             // VIERI
static int8_t WordStundeFuenf[] = {59, 58, 57, 56, -1};                // FÜFI
static int8_t WordStundeSechs[] = {68, 69, 70, 71, 72, 73, -1};        // SÄCHSI
static int8_t WordStundeSieben[] = {72, 73, 74, 75, 76, -1};           // SIBNI
static int8_t WordStundeAcht[] = {87, 86, 85, 84, 83, -1};             // ACHTI
static int8_t WordStundeNeun[] = {82, 81, 80, 79, -1};                 // NÜNI
static int8_t WordStundeZehn[] = {90, 91, 92, 93, -1};                 // ZÄNI
static int8_t WordStundeElf[] = {95, 96, 97, 98, -1};                  // EUFI
static int8_t WordStundeZwoelf[] = {106, 105, 104, 103, 102, 101, -1}; // ZWÖUFI

static int8_t *WordStunden[] = {WordStundeZwoelf, WordStundeEins, WordStundeZwei, WordStundeDrei, WordStundeVier,
                             WordStundeFuenf, WordStundeSechs, WordStundeSieben, WordStundeAcht, WordStundeNeun,
                             WordStundeZehn, WordStundeElf, WordStundeZwoelf
};

// Minute
static int8_t WordMinFuenf[] = {8, 9, 10, -1};                   // FÜF
static int8_t WordMinZehn[] = {14, 13, 12, -1};                  // ZÄÄ
static int8_t WordMinViertel[] = {21, 20, 19, 18, 17, 16, -1};   // VIERTU
static int8_t WordMinZwanzig[] = {22, 23, 24, 25, 26, 27, -1};   // ZWÄNZG
static int8_t WordMinTicks[] = {113, 114, 116, 117, -1};         // ** **

static int8_t *WordMinuten[] = {WordMinFuenf, WordMinZehn, WordMinViertel, WordMinZwanzig, WordMinFuenf};

// aktueller und letzter Zeit-Satz
int8_t satzalt[30];
int8_t satzneu[30];
uint8_t satzindex = 0;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numPixels, D7, NEO_GRB + NEO_KHZ800);

// How long this state should be displayed
int wifiWait = 0;

// Snake variables
int8_t snake[120];
uint8_t snakeLen = 3;
uint8_t snakeHighScore = 0;
int snakeNext = -1;
int snakeSnack = -2;  // pixel 0-120
String snakeDir = ""; // snake, up, right, down, left, stop
String snakePrevDir = "";
int snakeSpeed = 7000;
int snakeWait = 7000;
bool inSnake = false;

// Tetris variables
uint8_t board[11][11] = {0}; // 0 = empty, >0 = color index
int tetrisDir = 0; // 1=rotate, 2=right, 3=down, 4=left, 5=new game, 6=exit game
uint8_t tetrisScore = 0;
uint8_t tetrisHighScore = 0;
bool inTetris = false;
unsigned long lastDrop = 0;
const unsigned long dropInterval = 600; // ms

// Tetromino definitions (4x4 matrix for each shape)
const uint8_t tetrominos[7][4][4] = {
  // I
  {
    {0,0,0,0},
    {1,1,1,1},
    {0,0,0,0},
    {0,0,0,0}
  },
  // J
  {
    {0,0,0,0},
    {0,1,0,0},
    {0,1,1,1},
    {0,0,0,0}
  },
  // L
  {
    {0,0,0,0},
    {0,0,1,0},
    {1,1,1,0},
    {0,0,0,0}
  },
  // O
  {
    {0,0,0,0},
    {0,1,1,0},
    {0,1,1,0},
    {0,0,0,0}
  },
  // S
  {
    {0,0,0,0},
    {0,1,1,0},
    {1,1,0,0},
    {0,0,0,0}
  },
  // T
  {
    {0,0,0,0},
    {0,1,0,0},
    {1,1,1,0},
    {0,0,0,0}
  },
  // Z
  {
    {0,0,0,0},
    {1,1,0,0},
    {0,1,1,0},
    {0,0,0,0}
  }
};

// Current piece state
int currentTetromino, rotation, posX, posY;
uint8_t currentPiece[4][4]; // Store current piece with rotation applied
bool gameOver = false;

// Map (x, y) to LED index for serpentine wiring
uint8_t xyToIndex(uint8_t x, uint8_t y) {
  if (y % 2 == 0) {
    return y * 11 + x;
  } else {
    return y * 11 + (10 - x);
  }
}

// Hilfsfunktion: Prüft, ob LED in Array enthalten ist
bool inArray(uint8_t led, int8_t *arr) {
  for (uint8_t i = 0; arr[i] != -1; i++) {
    if (arr[i] == led) return true;
  }
  return false;
}

// Hilfsfunktion: HSV zu RGB 
uint32_t colorWheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return pixels.Color(255 - pos * 3, 0, pos * 3);
  }
  if (pos < 170) {
    pos -= 85;
    return pixels.Color(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return pixels.Color(pos * 3, 255 - pos * 3, 0);
}

// Hilfsfunktion: Dimm-Animation auf Schwarz
void dimToBlack(int led, uint32_t color, int steps, int delayMs) {
  for (int i = steps; i >= 0; i--) {
    float f = (float)i / steps;
    uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * f);
    uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * f);
    uint8_t b = (uint8_t)((color & 0xFF) * f);
    pixels.setPixelColor(led, pixels.Color(r, g, b));
    pixels.show();
    delay(delayMs);
  }
  pixels.setPixelColor(led, Black);
  pixels.show();
}

// Hilfsfunktion: Lichtimpuls-Animation
void pulseOn(int led, uint32_t color, int steps, int delayMs) {
  // Impuls heller als foregroundColor
  for (int i = 0; i <= steps; i++) {
    float f = 1.0 + 0.5 * sin(PI * i / steps); // von 1.0 bis 1.5 und zurück
    uint8_t r = min((uint8_t)255, (uint8_t)(((color >> 16) & 0xFF) * f));
    uint8_t g = min((uint8_t)255, (uint8_t)(((color >> 8) & 0xFF) * f));
    uint8_t b = min((uint8_t)255, (uint8_t)((color & 0xFF) * f));
    pixels.setPixelColor(led, pixels.Color(r, g, b));
    pixels.show();
    delay(delayMs);
  }
  pixels.setPixelColor(led, color);
  pixels.show();
}

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
// ESDISCHWFÜFYÄÄZTUTREIVZWÄNZGQDVORZTIBUAHDBAKEISQZWÖIDRÜTIFÜFIREIVZGMSÄCHSIBNIFOINÜNITHCACDZÄNIXEUFIXLIFUÖWZGKOLYB..P..MK.
const char* wordGuessrLetters = "ESDISCHWFuFYaaZTUTREIVZWaNZGQDVORZTIBUAHDBAKEISQZWoIDRuTIFuFIREIVZGMSaCHSIBNIFOINuNITHCACDZaNIXEUFIXLIFUoWZGKOLYB..P..MK.";
char wordGuessrLettersCopy[numPixels];

const char wordGuessrWords0[] PROGMEM = "AAL AARE ABEND ABENTEUER ABSCHLUSS ACHSE ACHSEL ACHT ACKER ADER ADLER ADRIA AFRIKA AGENT ÄGYPTEN AKTION ALPEN ALPHABET ALTER AMEISE AMERIKA AMOR AMT ANANAS ANFANG ANGEBOT ANGOLA ANGST ANLAGE ANTILOPE ANTWORT ANWALT ANZUG APFEL APRIKOSE APRIL ARBEIT ARCHIV ÄRGER ARM ARMBAND ARTIKEL ARZT ASPHALT ASTRONAUT ATHEN ATMEN AUGE AUGUST AUSDAUER AUSGANG AUSPUFF AUSWAHL AUTO AUTOBAHN AVOCADO BACH BACKE BADEN BADEWANNE BAHN BALL BANANE BAND BANK BÄR BART BASEL BATTERIE BAUCH BAUER BAUM BECHER BEDINGUNG BEERE BEGINN BEIL BEIN BEISPIEL BEITRAG BERATUNG BERG BERICHT BERLIN BERN BERUF BESCHWERDE BESUCH BETON BETRAG BETT BEWEIS BEWERTUNG BIBER BIBLIOTHEK BIENE BIER BILD BILLARD BILLIG"; 
const char wordGuessrWords1[] PROGMEM = "BIRNE BISON BLATT BLAU BLICK BLICKEN BLITZ BLÖD BLUME BLUT BLÜTE BODEN BOGEN BOHNE BOLIVIEN BOMBE BRAUE BRAUN BREI BREIT BRIEF BRILLE BROKKOLI BRONZE BROT BRÜCKE BRUST BUCH BUCHE BUCHSTABE BÜHNE BUND BURG BÜRSTE BUSCH BUTTER CHILE CHINA COMPUTER DACH DACHS DACKEL DAME DÄNEMARK DANK DARM DATTEL DAUER DAUMEN DECKE DELFIN DETAIL DETEKTIV DIALOG DICK DIEB DIENST DING DISTEL DOKTOR DOKUMENT DOMINO DONAU DONNER DOOF DORF DRACHE DRAMA DREI DREIECK DREIRAD DROMEDAR DROSSEL DUMM DUNKEL DÜNN DUNST EBBE EBENE ECHSE ECKE EICHE EIDECHSE EIER EINFLUSS EINKAUF EINS EINTOPF EIS EISBÄR EISHOCKEY EKLIG ELCH ELEFANT ELLBOGEN ELTERN ENDE ENERGIE ENKEL ENTE"; 
const char wordGuessrWords2[] PROGMEM = "ERBSE ERDE ERFINDUNG ERFOLG ERKLÄRUNG ERLEBNIS ERWARTUNG ESCHE ESEL ESSEN ESTRICH EULE EWIGKEIT EXTREM FADEN FAHRPLAN FAHRRAD FALKE FALLE FALTER FAMILIE FANTASIE FARBE FASS FEBRUAR FEE FEHLER FEIGE FELD FELS FENCHEL FENSTER FERNSEHER FEST FESTIVAL FESTUNG FEUER FEUERWEHR FILM FINALE FINGER FINK FINNLAND FIRMA FISCH FISCHER FLAMINGO FLAMME FLEISCH FLIEGE FLIEGEN FLOH FLOSS FLOSSE FLUGZEUG FLUSS FLUT FONDUE FORELLE FORM FORMAT FOTOGRAF FRAGE FRAU FREIHEIT FREITAG FREUDE FREUND FRIEDEN FRISUR FROSCH FROST FRUCHT FRÜHLING FRÜHSTÜCK FUCHS FÜNF FUNKTION FUSS FUSSBALL GABEL GANG GANS GARTEN GAS GASSE GAST GAZELLE GEBÄUDE GEBIET GEBISS GEBURT GEDANKE GEDICHT GEFAHR GEFÄNGNIS GEFÜHL GEHEIMNIS GEHEN GEHIRN"; 
const char wordGuessrWords3[] PROGMEM = "GEIER GEIST GELB GELD GELENK GEMÄLDE GEMÜSE GENF GEPARD GERICHT GESANG GESCHÄFT GESCHICHTE GESCHMACK GESICHT GESPENST GESPRÄCH GETREIDE GEWICHT GEWINN GIPFEL GIPS GIRAFFE GLARUS GLAS GLEIS GLOCKE GLÜCK GOLD GOLF GORILLA GOTT GRABEN GRAD GRAS GRENZE GRILL GROSS GRUBE GRÜN GRUND GUT HAAR HAGEL HAHN HAI HÄLFTE HALLE HALS HAMMER HAMSTER HAND HASE HASS HAUS HAUT HECHT HEFT HEIMAT HEISS HELL HELSINKI HERBST HERDE HERZ HILFE HIMBEERE HIMMEL HINTEN HINWEIS HIRSCH HIRT HITZE HOBBY HOCH HOCHHAUS HOCHZEIT HOFFNUNG HÖHLE HOLUNDER HOLZ HONIG HOSE HOTEL HÜFTE HÜGEL HUHN HUMMEL HUND HUNDERT HUT HÜTTE HYÄNE IGEL IMMER INDIANER INDIEN INHALT INSEL INTERESSE"; 
const char wordGuessrWords4[] PROGMEM = "INTERNET IRAK IRAN IRLAND ISLAND ITALIEN KABEL KÄFER KAFFEE KAIRO KAISER KAKAO KALB KALT KÄLTE KAMEL KAMERA KAMIN KAMM KAMPF KANADA KÄNGURU KANTON KANU KAROTTE KARTE KARTOFFEL KÄSE KASSE KATZE KAUF KELLE KELLER KELLNER KENIA KERN KERZE KETTE KIEFER KIES KIND KINO KIRCHE KIRSCHE KISSEN KIWI KLASSE KLEID KLEIDER KLEIN KLIMA KLINGE KNAST KNIE KNOBLAUCH KNOCHEN KNOSPE KOBRA KOFFER KOHL KOLIBRI KOLLEGE KOMET KOMMA KOMMODE KONFITÜRE KONFLIKT KONGO KÖNIG KONSTANT KONTAKT KONZERT KOREA KÖRPER KOSTEN KRAFT KRAFTWERK KRAKE KRANICH KRATER KREIS KREUZ KRIEG KRIMI KRISE KRITIK KROKODIL KRÖTE KÜCHE KUGEL KUH KÜKEN KULTUR KUNDE KUNST KÜNSTLER KUPFER KÜRBIS KURS KUSS"; 
const char wordGuessrWords5[] PROGMEM = "LACHEN LACHS LAGE LAMM LAND LANDKARTE LÄNGE LANGSAM LAUB LAUCH LAUS LAUT LAVA LEBEN LEHRER LEISE LEITER LEKTION LEOPARD LIBANON LICHT LIEBE LIED LIFT LINSE LISTE LITAUEN LOCH LÖFFEL LONDON LÖSUNG LÖWE LUCHS LUFT LUNGE LUST LUSTIG MÄDCHEN MADRID MAGEN MAI MAIS MANDARINE MANGO MANN MARKE MARKT MARS MÄRZ MASCHINE MATRATZE MAUER MAUS MEER MEINUNG MEISE MEISSEL MEISTER MELODIE MENGE MENSCH MESSER MESSING METALL METER METRO MEXIKO MILCH MINUTE MITTAG MITTE MITTEL MODE MOMENT MONAT MOND MONITOR MÖRDER MORGEN MÖVE MUMIE MUND MÜNSTER MÜNZE MUSCHEL MUSEUM MUSIK MUSKEL MUT MUTTER MÜTZE NACHT NACHTEIL NADEL NAGEL NAH NÄHE NAME NARBE NASE"; 
const char wordGuessrWords6[] PROGMEM = "NASHORN NASS NATUR NEBEL NEPAL NETZ NEUN NIERE NIGERIA NILPFERD NORDEN NOVEMBER NUDELN NUSS OBST OCHSE OHR OHRRING OKTOBER OLIVE OLYMPIA OMA OMELETTE ONKEL OPA OPER ORANGE ORDNER OSLO OSTEN PAKISTAN PALME PANDA PANDABÄR PARIS PARTEI PARTY PATIENT PAUSE PEKING PELIKAN PERSON PERU PFANNE PFEFFER PFEIL PFERD PFIRSICH PFLANZE PFLASTER PFLAUME PFLICHT PHARAO PILZ PINGUIN PINSEL PISTE PLAN PLANET POLEN POLITIK POLIZEI POLIZIST PORTUGAL POST PRACHT PRAG PRAXIS PREIS PRINZ PROBLEM PRODUKT PROFI PROGRAMM PROZENT PUDEL PUNKT PYRAMIDE QUALLE QUARTAL QUELLE QUITTE RABE RACLETTE RAD RADIO RADIUS RAKETE RASEN RÄTSEL RATTE RÄUBER RAUPE RAVIOLI RECHT REDEN REGEL REGEN REGION REH"; 
const char wordGuessrWords7[] PROGMEM = "REICH REIM REIS REISE RENNEN RENTIER REST RICHTER RICHTIG RITTER ROBBE ROBOTER ROCHEN ROM ROMAN ROSA ROT RUANDA RÜCKEN RUHE RÜHREI RUNDE SÄBEL SACHE SAFT SALAT SALZ SAMSTAG SAND SATURN SATZ SCHACH SCHÄDEL SCHAF SCHATTEN SCHAUFEL SCHENKEL SCHIFF SCHLÄFE SCHLAMM SCHLANGE SCHLAU SCHLECHT SCHLOSS SCHMAL SCHMUCK SCHNAUZ SCHNECKE SCHNEE SCHNEIDER SCHNELL SCHÖN SCHRANK SCHRAUBE SCHRIFT SCHRITT SCHROTT SCHUH SCHÜLER SCHULTER SCHUTZ SCHWAN SCHWEIN SCHWEIZ SCHWERT SECHS SEE SEHNE SEIL SEITE SEKUNDE SEMESTER SENDUNG SENSATION SICHT SIEBEN SIEG SILBE SILBER SILVESTER SIRUP SKELETT SKI SOCKE SOFA SOHN SOLDAT SOMMER SONNE SORGE SPAGAT SPARGEL SPASS SPATZ SPECHT SPEER SPEICHE SPEICHEL SPEISE SPIEL";
const char wordGuessrWords8[] PROGMEM = "SPINAT SPINNE SPION SPORT SPRACHE STAAT STADT STAND STAR START STECKER STEHEN STEIN STEINBOCK STELLE STERN STIEFEL STIER STIFT STIMME STIRN STOFF STORCH STRAND STRASSE STRAUSS STROH STROM STÜCK STUDIE STUHL STUNDE STURM SÜDEN SUMME SUMPF SUPER SYRIEN SZENE TAG TAL TANDEM TANKER TANNE TANSANIA TANTE TANZ TASTATUR TÄTER TAUCHER TAUSEND TEE TEENAGER TEER TEICH TEIL TELEFON TELLER TENNIS TERMIN TESSIN TEST TEUER TEXT THAILAND THEATER THRILLER THRON TIEF TIER TIGER TINTE TISCH TITEL TOCHTER TOD TOKYO TOMATE TOURIST TRAKTOR TRAM TRANK TRAUBE TRAUER TRAUM TRICK TRINKEN TROCKEN TROTTOIR TRUTHAHN TUCH TULPE TÜR TÜRKEI ÜBEL UGANDA UHR UHU UKRAINE UMFANG"; 
const char wordGuessrWords9[] PROGMEM = "UNFALL UNGARN UNIVERSUM UNKRAUT URI URIN URLAUB URSACHE URTEIL VANILLE VATER VEILCHEN VELO VENE VENEDIG VENUS VERBAND VEREIN VERSE VERTRAG VIDEO VIER VIETNAM VIOLETT VOGEL VOLK VORHANG VORNE VORTEIL VULKAN WADE WAGEN WAL WALD WAND WANDERUNG WARM WÄRME WASSER WECHSEL WEG WEIHNACHT WEIN WEIT WEIZEN WELT WERBUNG WERK WERKZEUG WERT WESPE WESTEN WETTER WIEN WIMPER WIND WINDELN WINKEL WINTER WIRKUNG WITZ WITZIG WOCHE WOLF WOLKE WOLLE WORT WUNDE WUNDER WUNSCH WURM WURST ZAHL ZANGE ZAUN ZEBRA ZEHN ZEICHEN ZEIT ZEITUNG ZEMENT ZIEGE ZIEGEL ZIEL ZINS ZITRONE ZOPF ZUCKER ZUG ZUKUNFT ZUNGE ZÜRICH ZUSTAND ZWECK ZWEI ZWEIFEL ZWIEBEL ZWÖLF ZYLINDER FALSCH"; 

const char* const wordGuessrWordsAll[] PROGMEM = {wordGuessrWords0, wordGuessrWords1, wordGuessrWords2, wordGuessrWords3, wordGuessrWords4, wordGuessrWords5, wordGuessrWords6, wordGuessrWords7, wordGuessrWords8, wordGuessrWords9};
char wordGuessrWordsBuffer[800]; // Buffer to load one of the 10 wordlists from progmem to ram; must be long enough for longest list

String wordGuessrActiveWord = "";
int8_t wordGuessrActiveWordIndex[20] = {}; // max 20 letters
int wordGuessrScore = 0;
unsigned long wordGuessrAlert = 0;
unsigned long wordGuessrStart = 0;
int wordGuessrHint = 0;
bool inWordGuessr = false;

// Ghost variables
int8_t ghostHour = 0;
int8_t ghostMinute = 0;
int ghostStep = 0;
int ghostChange = 1;
static int8_t WordGhost[] = {5, 6, 7, 17, 16, 15, 25, 26, 27, 28, 29, 40, 38, 36, 44, 47, 49, 51, 53, 54, 65, 64, 63, 62, 61, 59, 58, 57, 56, 55, 67, 68, 69, 70, 72, 73, 74, 75, 85, 84, 83, 82, 81, 80, 79, 91, 92, 93, 94, 95, 105, 104, 103, 102, 116, 117, 118, -1};
static int8_t WordGhostEyes[] = {39, 48, 37, 50, -1};
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
 * @param brightness float to dim color, 0.5 for 50% brightness
 */
uint32_t scaleColor(uint32_t color, float brightness) {
  uint8_t r = (uint8_t)((uint8_t)(color >> 16) * brightness);
  uint8_t g = (uint8_t)((uint8_t)(color >> 8) * brightness);
  uint8_t b = (uint8_t)((uint8_t)color * brightness);
  return Adafruit_NeoPixel::Color(r, g, b);
}

/**
 * Sets all pixels to the background-color
 */
void blank() {
  for (uint8_t x = 0; x < pixels.numPixels(); ++x) {
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
 * @param pixel int id of the pixel to move down
 * @param rows int number of rows to move donw
 */
int down(uint8_t pixel, uint8_t rows) {
  for (uint8_t i = 0; i < rows; i++) {
    pixel = pixel + 1 + 2 * (10 - pixel % 11);
  }
  return pixel;
}

/**
 * Adds a word to the new time-sentence
 * @param word array with the id's of the pixels
 */
void addword(int8_t *word) {
  for (uint8_t x = 0; x < pixels.numPixels() + 1; x++) {
    satzneu[satzindex] = (word[x]);
    if (word[x] == -1) {
      break;
    } else {
      satzindex ++;
    }
  }
}


/**
 * Sets array of pixels to a specific color
 * @param word array with the id's of the pixels
 * @param color Adafruit_NeoPixel-Color to display those pixels
 */
void lightup(int8_t *word, uint32_t color) {
  for (uint8_t x = 0; x < pixels.numPixels() + 1; x++) {
    if (word[x] == -1) {
      break;
    } else {
      pixels.setPixelColor(word[x], color);
    }
  }
}

// Initialisiere Drop in Spalte
void initDrop(uint8_t col, uint8_t dropIndex) {
  drops[col][dropIndex].y = -random(1, 8); // Startet oberhalb der Matrix
  drops[col][dropIndex].speed = 0.1 + random(5, 80) / 100.0; // 0.15..0.90
  drops[col][dropIndex].length = random(3, 8); // 3..7
  drops[col][dropIndex].maxBrightness = random(120, 255); // 120..255
  drops[col][dropIndex].active = true;
}

// Matrix-Animation
void matrixEffect() {
  lightup(satzalt, foregroundColor);
  frame = 0;
  for (uint8_t x = 0; x < 11; x++) {
    initDrop(x, 0);
    dropCooldown[x] = 0;
  }
  while (true) {
    frame++;
    bool allowNewDrops = (frame < effectSpeed / 50); // Dauer der Animation (effectSpeed 8-7808 in Arduino)
    
    bool hasActiveDrops = false;
    
    for (uint8_t x = 0; x < 11; x++) {
      // Cooldown für neue Drops verwalten
      if (dropCooldown[x] > 0) {
        dropCooldown[x]--;
      }
      
      // Alle Drops in dieser Spalte verarbeiten
      for (uint8_t d = 0; d < 2; d++) {
        if (!drops[x][d].active) {
          // Wenn dieser Drop inaktiv ist und Cooldown abgelaufen, neuen Drop starten
          if (dropCooldown[x] == 0 && allowNewDrops) {
            initDrop(x, d);
            dropCooldown[x] = random(5, 30); // Kurzer Cooldown bis zum nächsten Drop
          }
          continue;
        }
        
        hasActiveDrops = true;
        drops[x][d].y += drops[x][d].speed;
        
        // Drop ist fertig, wenn er unten angekommen ist
        if (drops[x][d].y - drops[x][d].length > 11) {
          drops[x][d].active = false;
          continue;
        }
        
        // Zeichne den Drop
        for (int8_t i = 0; i <= drops[x][d].length; i++) {
          int8_t py = (int8_t)(drops[x][d].y) - i;
          if (py < 0 || py > 11) continue;
          uint16_t idx = xyToIndex(x, py);
          
          // Prüfe, ob dieser Pixel in satzneu enthalten ist
          if (inArray(idx, satzneu)) {
            // Satzneu-Pixel: keine Fade, volle Helligkeit
            pixels.setPixelColor(idx, foregroundColor);
          } else {
            // Andere Pixel: normales Faden
            float fade = 1.0 - (float)i / drops[x][d].length;
            fade = fade * fade; // Exponentieller Fade für stärkeren Effekt
            uint8_t brightness = (uint8_t)(drops[x][d].maxBrightness * fade);
            uint32_t col = pixels.Color(
              (uint8_t)((foregroundColor >> 16) & 0xFF) * brightness / 255,
              (uint8_t)((foregroundColor >> 8) & 0xFF) * brightness / 255,
              (uint8_t)(foregroundColor & 0xFF) * brightness / 255
            );
            pixels.setPixelColor(idx, col);
          }
        }
      }
    }
    
    pixels.show();
    delay(30);
    
    // Schleife nur beenden wenn keine Drops mehr aktiv sind und neue Drops nicht mehr gespawnt werden
    if (!allowNewDrops && !hasActiveDrops) break;
  }
}

// Pulse-Animation
void pulseEffect() {
  float radius = -2.0f;
  const float speed = 0.10f; // Geschwindigkeit der Welle
  const float maxRadius = 9.0f; // maximaler Abstand von Mitte zu Ecke
  const float width = 1.1f; // Breite der Welle
  const uint8_t cx = 5, cy = 5; // Mittelpunkt

  lightup(satzalt, foregroundColor);

  // Statusarrays: markiere satzalt und satzneu Pixel und ob ihr Peak erreicht wurde
  bool peakReached[numPixels];
  for (uint16_t i = 0; i < numPixels; ++i) {
    peakReached[i] = false;
  }
  const float peakThreshold = 0.95f; // Schwelle um "Maximalhelligkeit" zu erkennen
  const float peakEpsilon = 0.05f; // wie nah an diff==0 gelten wir als Peak

  while (radius < maxRadius + width * 3.0f) {
    for (uint8_t y = 0; y < 11; y++) {
      for (uint8_t x = 0; x < 11; x++) {
        float dx = x - cx;
        float dy = y - cy;
        float dist = sqrtf(dx * dx + dy * dy);
        float diff = dist - radius;
        float brightness = expf(-0.5f * (diff * diff) / (width * width));
        if (brightness < 0.01f) brightness = 0.0f;
        uint16_t idx = xyToIndex(x, y);

        // wurde Peak erreicht?
        if (!peakReached[idx] && (fabsf(diff) <= peakEpsilon || brightness >= peakThreshold)) peakReached[idx] = true;

        if (inArray(idx, satzalt) && !peakReached[idx]) {
          // nichts tun, bleibt hell bis Peak
        } else if (inArray(idx, satzneu) && peakReached[idx]) {
          // satzneu nach Peak: dauerhaft weiß
          pixels.setPixelColor(idx, foregroundColor);
        } else {
          // normale Pixel folgen der Welle
          pixels.setPixelColor(idx, scaleColor(foregroundColor, brightness));
        }
      }
    }
    pixels.show();
    radius += speed;
    // effectSpeed 8-7808 in Arduino
    delay(effectSpeed / 400);
  }
}

// Typewriter Animation
void typewriterEffect() {
  lightup(satzalt, foregroundColor);
  // 1. Löschen: alle LEDs, die nur in satzalt sind
  for (int8_t y = 10; y >= 0; y--) {
    for (int8_t x = 10; x >= 0; x--) {
      int idx = xyToIndex(x, y);
      if (inArray(idx, satzalt) && !inArray(idx, satzneu)) {
        dimToBlack(idx, foregroundColor, 8, effectSpeed / 200);
      }
    }
  }
  delay(effectSpeed / 20);
  // 2. Einschalten: alle LEDs, die nur in satzneu sind
  for (int8_t y = 0; y < 11; y++) {
    for (int8_t x = 0; x < 11; x++) {
      int idx = xyToIndex(x, y);
      if (inArray(idx, satzneu) && !inArray(idx, satzalt)) {
        pulseOn(idx, foregroundColor, 8, effectSpeed / 200);
      }
    }
  }
}

/**
 * Get the numeric value of a URL-Parameter
 * @param url char url-string
 * @param paramName char name of parameter to search
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
    addword(WordStunden[wordClockHour % 12]);
  } else {
    // show next hour
    addword(WordStunden[(wordClockHour % 12) + 1]);
  }
}

/**
 * Sets pixels for the current minutes
 */
void showMinute() {
  if (wordClockMinute != 0) {
    if (wordClockMinute >= 5 && wordClockMinute < 30) {
      // sets pixels with minutes array
      addword(WordMinuten[(wordClockMinute / 5) - 1]);
    } else if (wordClockMinute >= 35) {
      // sets pixels with same array, but in reverse
      addword(WordMinuten[5 - ((wordClockMinute - 30) / 5)]);
    }
    if ((wordClockMinute >= 5 && wordClockMinute < 25) || (wordClockMinute < 40 && wordClockMinute >= 35)) {
      addword(WordNach);
    }
    if (wordClockMinute >= 40 || (wordClockMinute >= 25 && wordClockMinute < 30)) {
      addword(WordVor);
    }
    if (wordClockMinute >= 25 && wordClockMinute < 40) {
      addword(WordHalb);
    }
    // Checks if the minute ticks should be displayed
    int differenceToLast5Min = wordClockMinute % 5;
    for (uint8_t i = 0; i < differenceToLast5Min; i++) {
      //pixels.setPixelColor(WordMinTicks[i], foregroundColor);
      satzneu[satzindex] = (WordMinTicks[i]);
      satzindex ++;
      satzneu[satzindex] = -1;
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

void setForegroundColor() {
  // Display darker color between 22:00 and 07:00
  if (darkMode == 1 && (wordClockHour >= 22 || wordClockHour < 7)) {
    foregroundColor = colorNight;
  } else {
    foregroundColor = colorDay;
  }
}

/**
 * Displays the current time
 */
void displayTime() {
  satzindex = 0;
  memcpy(satzalt, satzneu, sizeof(satzneu));
  blank();
  setForegroundColor();

  // light up "it's" it stays on
  addword(WordEs);
  addword(WordIst);

  showMinute();
  showHour();

  if (effect == 2) {
    // Rainbow effect
    for (uint8_t y = 0; y < 11; y++) {
      for (uint8_t x = 0; x < 11; x++) {
        uint16_t idx = xyToIndex(x, y);
        if (inArray(idx, satzneu)) {
          // Diagonale bestimmen: x + y
          uint8_t diag = x + y;
          // Regenbogenfarbe berechnen, animiert durch frame
          uint8_t colorPos = (diag * 20 + frame) % 256;
          uint32_t color = colorWheel(colorPos);
          pixels.setPixelColor(idx, color);
        }
      }
    }
  } else if (effect == 3 && wordClockMinute % 5 == 0) {
    // Matrix effect
    matrixEffect();
  } else if (effect == 4 && wordClockMinute % 5 == 0) {
    // Pulse effect
    pulseEffect();
  } else if (effect == 5) {
    // Typewriter effect
    typewriterEffect();
  } else {
    // No effect
    lightup(satzneu, foregroundColor);
  }

  displayWifiStatus();

  pixels.show();
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

void sendProgmemString(WiFiClient& client, const char* str) {
  const size_t CHUNK_SIZE = 1024; // Größe der Chunks anpassen zwischen 256-1024 anpassen, je nach verfügbarem RAM
  char buffer[CHUNK_SIZE];
  size_t len = strlen_P(str);
  size_t pos = 0;

  while (pos < len) {
    size_t remaining = len - pos;
    size_t toRead = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;
    memcpy_P(buffer, str + pos, toRead);
    client.write((const uint8_t*)buffer, toRead);
    pos += toRead;
    yield(); // Gibt dem ESP8266 Zeit für WiFi-Operationen
  }
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
  lightup(WordFix, White);
  lightup(WordWifi, Blue);
  pixels.show();

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name (wordclock)
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect(version);
  
  // or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();


  // or connect to WiFi with hardcoded name/pw:
  // WiFi.begin("WiFiName", "WiFiPassword");
  // Serial.print(F("Connecting"));
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(500);
  //   Serial.print(F("."));
  // }
  // Serial.println();


  // if you get here you have connected to the WiFi
  Serial.println(F("Connected. IP address: "));
  Serial.println(WiFi.localIP());
  // Display local ip address on clockface by looping through every character.
  String localIP = WiFi.localIP().toString();
  for (uint8_t i = 0; i < localIP.length(); i++) {
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
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  chase(Green);
}

// send params to all connected WebSocket clients
void sendParamsToClients() {
  String msg;
  msg.reserve(100);
  msg = "{";
  msg += "\"red\":" + String(rgbRed);
  msg += ", \"green\":" + String(rgbGreen);
  msg += ", \"blue\":" + String(rgbBlue);
  msg += ", \"effect\":" + String(effect);
  msg += ", \"darkmode\":" + String(darkMode);
  msg += ", \"speed\":" + String(effectSpeed / 4 + 48);
  msg += ", \"power\":" + String(power);
  msg += ", \"ghost\":" + String(ghost);
  msg += "}";
  webSocket.broadcastTXT(msg);
}

/*
 * WebSocket event handler: receive control commands from client
 */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_CONNECTED) {
    // send current score on new connection
    sendParamsToClients();
    return;
  }
  if (type != WStype_TEXT) return;
  String msg = String((char*)payload);
  if (msg == "tetris") {
    // Tetris start
    inTetris = true;
    inMastermind = false;
    inWordGuessr = false;
    inSnake = false;
    blank();
    pixels.show();
    handleRestart();
    sendScoreToClients(0, tetrisHighScore);
  } else if (msg == "snake") {
    // Snake start
    inSnake = true;
    inMastermind = false;
    inWordGuessr = false;
    inTetris = false;
    snake[0] = 49;
    snake[1] = 60;
    snake[2] = 71;
    snake[3] = -1;
    snakeLen = 3;
    snakeDir = "";
    snakeNext = -1;
    snakeSpeed = 7000;
    sendScoreToClients(0, snakeHighScore);
    blank();
    lightup(snake, Green);
    setSnack();
    pixels.show();
  } else if (msg == "stop") {
     // Tetris or Snake exit
    inTetris = false;
    inSnake = false;
    lastMinuteWordClock = 61;
  } else if (inSnake) {
    snakePrevDir = snakeDir;
    snakeDir = msg;
  } else if (inTetris && msg == "left") {
    if (!checkCollision(posX - 1, posY, rotation)) { posX--; if(!gameOver) drawBoard(); }
  } else if (inTetris && msg == "right") {
    if (!checkCollision(posX + 1, posY, rotation)) { posX++; if(!gameOver) drawBoard(); }
  } else if (inTetris && msg == "up") {
    rotateTetromino();
  } else if (inTetris && msg == "down") {
    if (!checkCollision(posX, posY + 1, rotation)) {
      posY++;
    } else {
      placeTetromino();
      clearLines();
      spawnTetromino();
      if (checkCollision(posX, posY, rotation)) gameOver = true;
    }
    if (!gameOver) drawBoard();
  } 
}

/*
 * Snake: places a snack on an empty space in snake game
 */
void setSnack() {
  snakeSnack = -1;
  while (snakeSnack < 0) {
    snakeSnack = random(numPixels);
    for (int8_t i = snakeLen - 1; i >= 0; i--) {
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

// Tetris & Snake: Send current score to all connected WebSocket clients
void sendScoreToClients(uint8_t score, uint8_t highScore) {
  String msg = "{\"score\":" + String(score) + ", \"high\":" + String(highScore) + "}";
  webSocket.broadcastTXT(msg);
}

// Tetris: Spawn a new tetromino at the top
void spawnTetromino() {
  currentTetromino = random(0, 7);
  rotation = 0;
  posX = 3; // Centered
  posY = -2;  // Spawn above visible area
  // Copy initial tetromino to current piece
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t j = 0; j < 4; j++) {
      currentPiece[i][j] = tetrominos[currentTetromino][i][j];
    }
  }
  lastDrop = millis();  // Reset timer so piece has time to display before first drop
}

// Tetris: Check collision for current piece at (x, y) with rotation
bool checkCollision(int x, int y, int rot) {
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t j = 0; j < 4; j++) {
      if (currentPiece[i][j]) {
        int nx = x + j;
        int ny = y + i;
        // Only check collisions with boundaries and board for visible area
        if (nx < 0 || nx >= 11) return true;
        if (ny >= 11) return true;  // Piece hit bottom
        if (ny >= 0 && board[ny][nx]) return true;  // Only check board collision if visible
      }
    }
  }
  return false;
}

// Tetris: Place current piece on the board
void placeTetromino() {
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t j = 0; j < 4; j++) {
      if (currentPiece[i][j]) {
        int nx = posX + j;
        int ny = posY + i;
        if (nx >= 0 && nx < 11 && ny >= 0 && ny < 11) {
          board[ny][nx] = currentTetromino + 1;
        }
      }
    }
  }
}

// Tetris: Clear full lines and animate
void clearLines() {
  for (uint8_t y = 0; y < 11; y++) {
    bool full = true;
    for (uint8_t x = 0; x < 11; x++) {
      if (!board[y][x]) {
        full = false;
        break;
      }
    }
    if (full) {
      // Animate line
      for (uint8_t t = 0; t < 3; t++) {
        for (uint8_t x = 0; x < 11; x++) {
          pixels.setPixelColor(xyToIndex(x, y), White);
        }
        pixels.show();
        delay(80);
        for (uint8_t x = 0; x < 11; x++) {
          pixels.setPixelColor(xyToIndex(x, y), 0);
        }
        pixels.show();
        delay(80);
      }
      // Remove line and shift down
      for (uint8_t yy = y; yy > 0; yy--) {
        for (uint8_t x = 0; x < 11; x++) {
          board[yy][x] = board[yy-1][x];
        }
      }
      for (uint8_t x = 0; x < 11; x++) board[0][x] = 0;
      tetrisScore += 1;
      if (tetrisScore > tetrisHighScore) {
        tetrisHighScore = tetrisScore;
      }
      sendScoreToClients(tetrisScore, tetrisHighScore);
    }
  }
}

// Tetris: Draw the board and current piece
void drawBoard() {
  pixels.clear();
  // Draw placed blocks
  for (uint8_t y = 0; y < 11; y++) {
    for (uint8_t x = 0; x < 11; x++) {
      if (board[y][x]) {
        pixels.setPixelColor(xyToIndex(x, y), GameColors[board[y][x]-1]);
      }
    }
  }
  // Draw current piece
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t j = 0; j < 4; j++) {
      if (currentPiece[i][j]) {
        int nx = posX + j;
        int ny = posY + i;
        if (nx >= 0 && nx < 11 && ny >= 0 && ny < 11) {
          pixels.setPixelColor(xyToIndex(nx, ny), GameColors[currentTetromino]);
        }
      }
    }
  }
  pixels.show();
}

// Tetris: Rotate tetromino (clockwise)
void rotateTetromino() {
  uint8_t rotated[4][4];
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t j = 0; j < 4; j++) {
      rotated[j][3-i] = currentPiece[i][j];
    }
  }
  // Check collision for rotated piece
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t j = 0; j < 4; j++) {
      if (rotated[i][j]) {
        int nx = posX + j;
        int ny = posY + i;
        if (nx < 0 || nx >= 11 || ny >= 11 || (ny >= 0 && board[ny][nx])) {
          return; // Collision, do not rotate
        }
      }
    }
  }
  // Apply rotation
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t j = 0; j < 4; j++) {
      currentPiece[i][j] = rotated[i][j];
    }
  }
  drawBoard();
}

// Tetris: Restart the game
void handleRestart() {
  memset(board, 0, sizeof(board));
  tetrisScore = 0;
  gameOver = false;
  spawnTetromino();
  drawBoard();
}

/*
 * Wordguessr: find a random index of a letter in the wordGuessrLetters, return -1 if letter is not in the word
 * @param letter the letter to find
 * @param allLetters the string to search in
 */
int wordGuessrFindIndex(char letter) {
  int indices[16]; // max 16 occurences of a letter (i has 15)
  int count = 0;
  for (uint8_t j = 0; j < numPixels; j++) {
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
 * Wordguessr: select a new word to guess
 */
void wordGuessrNewGuess() {
  randomSeed(micros());

  int listNr = random(10); // Index to randomly load one of ten wordlists from progmem
  strcpy_P(wordGuessrWordsBuffer, (char*)pgm_read_ptr(&(wordGuessrWordsAll[listNr]))); 

  // Zähle, wie viele Wörter vorhanden sind
  int wordCount = 0;
  char* wordGuessrWords[100];  // Array to store list of words (max 100)

  // use strtok, to separate the words
  char* token = strtok(wordGuessrWordsBuffer, " ");
  while (token != NULL) {
    wordGuessrWords[wordCount++] = token;  
    token = strtok(NULL, " ");
  }

  int randomIndex = random(wordCount);
  wordGuessrActiveWord = wordGuessrWords[randomIndex];
  //Serial.print(F("New word to guess: "));
  //Serial.println(wordGuessrActiveWord);

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
 * Mastermind: prepares a new mastermind game
 */
void clearMastermind() {
  wipe();
  inMastermind = true;
  inWordGuessr = false;
  inSnake = false;
  inTetris = false;
  randomSeed(micros());
  mastermindCode[0] = random(1,7);
  mastermindCode[1] = random(1,7);
  mastermindCode[2] = random(1,7);
  mastermindCode[3] = random(1,7);
  mastermindTry = 0;
  mastermindPlace = 0;
  mastermindColor = 0;
  for (uint8_t i = 0; i < 11; i++) {
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
  pixels.begin();
  wipe();

  pinMode(D5, INPUT);

  // Initialize EEPROM and read stored values
  EEPROM.begin(512);

  uint8_t storedValue = EEPROM.read(eepromAddrRed);
  if (storedValue <= 255) {
    rgbRed = storedValue;
  }

  storedValue = EEPROM.read(eepromAddrGreen);
  if (storedValue <= 255) {
    rgbGreen = storedValue;
  }

  storedValue = EEPROM.read(eepromAddrBlue);
  if (storedValue <= 255) {
    rgbBlue = storedValue;
  }
  colorDay = Adafruit_NeoPixel::Color(rgbRed / 5, rgbGreen / 5, rgbBlue / 5);
  colorNight = Adafruit_NeoPixel::Color(rgbRed / 25, rgbGreen / 25, rgbBlue / 25);

  storedValue = EEPROM.read(eepromAddrDarkMode);
  if (storedValue == 0 || storedValue == 1) {
    darkMode = storedValue;
  }

  storedValue = EEPROM.read(eepromAddrGhost);
  if (storedValue == 0 || storedValue == 1) {
    ghost = storedValue;
  }

  storedValue = EEPROM.read(eepromAddrEffect);
  if (storedValue <= 5) {
    effect = storedValue;
  }

  storedValue = EEPROM.read(eepromAddrSnakeHigh);
  if (storedValue < 255) {
    snakeHighScore = storedValue;
  }

  storedValue = EEPROM.read(eepromAddrTetrisHigh);
  if (storedValue < 255) {
    tetrisHighScore = storedValue;
  }

  int storedEffectSpeed = (EEPROM.read(eepromAddrEffectSpeedHigh) << 8) | EEPROM.read(eepromAddrEffectSpeedLow);
  if (storedEffectSpeed >= 8 && storedEffectSpeed <= 7808) {
    effectSpeed = storedEffectSpeed;
  }

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
  webSocket.loop();
  if (client) {                             // If a new client connects,
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        //Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            if (header.indexOf("mastermind") >= 0) {
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
                for (uint8_t i = 0; i < 4; i++) {
                  pixels.setPixelColor(down(i + 1, mastermindTry), GameColors[mastermindCodeTry[i] - 1]);
                  mastermindCodeBackup[i] = mastermindCode[i];
                  // check right position
                  if (mastermindCodeTry[i] == mastermindCodeBackup[i]) {
                    mastermindCodeTry[i] = -1;
                    mastermindCodeBackup[i] = -2;
                    mastermindPlace ++;
                    pixels.setPixelColor(down(mastermindPlace + 5, mastermindTry), White);
                  }
                }
                for (uint8_t i = 0; i < 4; i++) {
                  for (uint8_t j = 0; j < 4; j++) {
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
                for (uint8_t i = 0; i < mastermindTry; i++) {
                  pixels.setPixelColor(down(0, i), Green);
                  pixels.setPixelColor(down(5, i), Green);
                  pixels.setPixelColor(down(10, i), Green);
                }
              } else if (mastermindTry == 11) {
                // player lost
                for (uint8_t i = 0; i < mastermindTry; i++) {
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
                  inTetris = false;
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
              } else {
                ghost = 0;
              }
              // Save ghost to EEPROM if it changed
              uint8_t storedGhost = EEPROM.read(eepromAddrGhost);
              if (storedGhost != ghost) {
                EEPROM.write(eepromAddrGhost, ghost);
                EEPROM.commit();
              }
              if (extractParameterValue(url, "power=") == 1) {
                power = 1;
              } else {
                power = 0;
                blank();
                pixels.show();
              }
              if (extractParameterValue(url, "speed=") >= 50 && extractParameterValue(url, "speed=") <= 2000) {
                effectSpeed = (extractParameterValue(url, "speed=") - 48) * 4; // map 50-2000 from WebParameter to 8-7808 in Arduino
              }
              // Save effectSpeed to EEPROM if it changed
              int storedEffectSpeed = (EEPROM.read(eepromAddrEffectSpeedHigh) << 8) | EEPROM.read(eepromAddrEffectSpeedLow);
              if (storedEffectSpeed != effectSpeed) {
                EEPROM.write(eepromAddrEffectSpeedLow, effectSpeed & 0xFF);
                EEPROM.write(eepromAddrEffectSpeedHigh, (effectSpeed >> 8) & 0xFF);
                EEPROM.commit();
              }
              if (extractParameterValue(url, "darkmode=") == 1) {
                darkMode = 1;
              } else {
                darkMode = 0;
              }
              // Save darkMode to EEPROM if it changed
              uint8_t storedDarkMode = EEPROM.read(eepromAddrDarkMode);
              if (storedDarkMode != darkMode) {
                EEPROM.write(eepromAddrDarkMode, darkMode);
                EEPROM.commit();
              }
              if (extractParameterValue(url, "effect=") >= 0 && extractParameterValue(url, "effect=") <= 5) {
                effect = extractParameterValue(url, "effect=");
              }
              // Save effect to EEPROM if it changed
              uint8_t storedEffect = EEPROM.read(eepromAddrEffect);
              if (storedEffect != effect) {
                EEPROM.write(eepromAddrEffect, effect);
                EEPROM.commit();
              }
              if (extractParameterValue(url, "blue=") >= 0 && extractParameterValue(url, "blue=") <= 255) {
                rgbBlue = extractParameterValue(url, "blue=");
              }
              // Save rgbBlue to EEPROM if it changed
              uint8_t storedBlue = EEPROM.read(eepromAddrBlue);
              if (storedBlue != rgbBlue) {
                EEPROM.write(eepromAddrBlue, rgbBlue);
                EEPROM.commit();
              }
              if (extractParameterValue(url, "green=") >= 0 && extractParameterValue(url, "green=") <= 255) {
                rgbGreen = extractParameterValue(url, "green=");
              }
              // Save rgbGreen to EEPROM if it changed
              uint8_t storedGreen = EEPROM.read(eepromAddrGreen);
              if (storedGreen != rgbGreen) {
                EEPROM.write(eepromAddrGreen, rgbGreen);
                EEPROM.commit();
              }
              if (extractParameterValue(url, "red=") >= 0 && extractParameterValue(url, "red=") <= 255) {
                rgbRed = extractParameterValue(url, "red=");
              }
              // Save rgbRed to EEPROM if it changed
              uint8_t storedRed = EEPROM.read(eepromAddrRed);
              if (storedRed != rgbRed) {
                EEPROM.write(eepromAddrRed, rgbRed);
                EEPROM.commit();
              }
              colorDay = Adafruit_NeoPixel::Color(rgbRed / 5, rgbGreen / 5, rgbBlue / 5);
              colorNight = Adafruit_NeoPixel::Color(rgbRed / 25, rgbGreen / 25, rgbBlue / 25);
              setForegroundColor();
              if (effect == 1) {
                // colorWheel
                rgbRed = 255;
                rgbGreen = 0;
                rgbBlue = 0;
                effectWait = effectSpeed;
              } else if (effect == 2) {
                // rainbow
                effectWait = effectSpeed / 8;
              } else if (effect == 3 && wordClockMinute % 5 != 0 && power == 1) {
                // matrix
                memcpy(satzalt, satzneu, sizeof(satzneu));
                matrixEffect();
              } else if (effect == 4 && wordClockMinute % 5 != 0 && power == 1) {
                // pulse
                memcpy(satzalt, satzneu, sizeof(satzneu));
                pulseEffect();
              } else if (effect == 5 && power == 1) {
                // typewriter
                satzalt[6] = -1; 
                blank();
                lightup(satzalt, foregroundColor);
                pixels.show();
                typewriterEffect();
              }
              lastMinuteWordClock = 61;
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-type:text/plain"));
              client.println(F("Connection: close"));
              client.println();
              client.println(F("OK"));
              // send current params to all connected WebSocket clients
              sendParamsToClients();
            } else {
              // New connection, send web interface to client:
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-type:text/html"));
              client.println(F("Connection: close"));
              client.println();

              // Display the HTML web page from PROGMEM
              sendProgmemString(client, web_interface);
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

  // Touch sensor to toggle power
  if (digitalRead(D5) == LOW && lastTouchStage == true) {
    power = 1 - power; // toggle power
    sendParamsToClients();
    if (power == 0) {
      blank();
      pixels.show();
    } else {
      lastMinuteWordClock = 61;
      if (effect == 3 && wordClockMinute % 5 != 0) {
        // matrix
        satzalt[0] = -1;
        matrixEffect();
      } else if (effect == 4 && wordClockMinute % 5 != 0) {
        // pulse
        satzalt[0] = -1;
        pulseEffect();
      } else if (effect == 5 && power == 1) {
        // typewriter
        satzalt[0] = -1; 
        typewriterEffect();
      }
    }
  }
  lastTouchStage = digitalRead(D5);

  // sleep and return when power off
  if (power == 0) {
    return;
  }

  // display ghost if enabled
  if (ghost == 1 && ghostHour == wordClockHour && ghostMinute == wordClockMinute) {
    if (!inGhost) {
      inGhost = true;
      // loop 11 Zeilen
      for (int8_t j = 11; j >= 0; j--) {
        blank();
        // Schleife durch das Array
        for (int8_t i = 0; i < numPixels; i++) {
          int ghostPixel = WordGhost[i];
          // Wenn der Wert -1 erreicht wird, die Schleife beenden
          if (WordGhost[i] == -1) {
            break;
          }
          // pixel um anzahl Zeilen nach unten verschieben
          ghostPixel = down(ghostPixel,j);
          pixels.setPixelColor(ghostPixel, scaleColor(White, (11 - (float)j) / 11));
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
      int tempRed   = (120 * ghostStep) / 100;    // Red value from 0 to 100
      int tempBlue  = (6 * ghostStep) / 100;      // Blue value from 0 to 5
    
      lightup(WordGhostEyes, pixels.Color(tempRed, 0, tempBlue));
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
      for (int8_t j = 0; j < 11; j++) {
        blank();
        // Schleife durch das Array
        for (int8_t i = 0; i < numPixels; i++) {
          int ghostPixel = WordGhost[i];
          // Wenn der Wert -1 erreicht wird, die Schleife beenden
          if (WordGhost[i] == -1) {
            break;
          }
          // pixel um anzahl Zeilen nach unten verschieben
          ghostPixel = down(ghostPixel,j);
          pixels.setPixelColor(ghostPixel, scaleColor(White, (11 - (float)j) / 11));
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
      if (snakeDir == "up") {
        snakeNext = snake[0] - 1 - 2 * (snake[0] % 11);
        if (snakeNext < 0) {
          snakeNext = -3;
        }
      } else if (snakeDir == "right") {
        snakeNext = snake[0] + 1 - 2 * ((snake[0] / 11) % 2);
        if (floor(snakeNext / 11) != snake[0] / 11) {
          snakeNext = -3;
        }
      } else if (snakeDir == "down") {
        snakeNext = snake[0] + 1 + 2 * (10 - snake[0] % 11);
        if (snakeNext > 120) {
          snakeNext = -3;
        }
      } else if (snakeDir == "left") {
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
      for (int8_t i = snakeLen - 1; i > 1; i--) {
        if (snakeNext == snake[i]) {
          // bite own tail
          snakeNext = -3;
        }
      }
      if (snakeNext == snakeSnack) {
        // found snack
        snakeLen++;
        if (snakeLen - 3 > snakeHighScore) {
          snakeHighScore = snakeLen - 3;
        }
        sendScoreToClients(snakeLen - 3, snakeHighScore);
        snake[snakeLen] = -1;
        setSnack();
        snakeSpeed = snakeSpeed - 40;
      }

      if (snakeNext >= 0) {
        // move snake one step forward
        for (int8_t i = snakeLen - 1; i > 0; i--) {
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
        webSocket.broadcastTXT("gameOver");
        if (snakeHighScore > EEPROM.read(eepromAddrSnakeHigh)) {
          EEPROM.write(eepromAddrSnakeHigh, snakeHighScore);
          EEPROM.commit();
        }
        chase(Red);
        inSnake = false;
        lastMinuteWordClock = 61;
      }

    }
  } else if (inTetris) {
    if (!gameOver && millis() - lastDrop > dropInterval) {
      lastDrop = millis();
      if (!checkCollision(posX, posY + 1, rotation)) {
        posY++;
      } else {
        placeTetromino();
        clearLines();
        spawnTetromino();
        if (checkCollision(posX, posY, rotation)) {
          gameOver = true;
        }
      }
      if (!gameOver) {
        drawBoard();
      }
    }
    if (gameOver) {
      delay(500);
      webSocket.broadcastTXT("gameOver");
      if (tetrisHighScore > EEPROM.read(eepromAddrTetrisHigh)) {
        EEPROM.write(eepromAddrTetrisHigh, tetrisHighScore);
        EEPROM.commit();
      }
      chase(Red);
      inTetris = false;
      lastMinuteWordClock = 61;
    }
  } else if (inWordGuessr) {
    if (wordGuessrAlert > 0 && wordGuessrAlert < millis()) {
      // nach Alert (richtig/falsch) wieder auf normale Anzeige wechseln
      blank();
      lightup(wordGuessrActiveWordIndex, foregroundColor);
      for (uint8_t i = 0; i < wordGuessrHint; i++) {
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
  } else if (effect == 1) {
    // colorWheel
    if (effectWait > 0) {
      effectWait--;
    } else {
      effectWait = effectSpeed;
      if (rgbRed > 0 && rgbBlue == 0) {
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
  } else if (effect == 2) {
    // rainbow
    if (effectWait > 0) {
      effectWait--;
    } else {
      effectWait = effectSpeed / 8;
      frame++;
      lastMinuteWordClock = 61;
    }
  }

  if (timeStatus() != timeNotSet && !inSnake && !inMastermind && !inWordGuessr && !inTetris) {
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
