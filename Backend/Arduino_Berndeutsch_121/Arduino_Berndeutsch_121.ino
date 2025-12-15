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
static int WordStundeSechs[] = {68, 69, 70, 71, 72, 73, -1};        // SÄCHSI
static int WordStundeSieben[] = {72, 73, 74, 75, 76, -1};           // SIBNI
static int WordStundeAcht[] = {87, 86, 85, 84, 83, -1};             // ACHTI
static int WordStundeNeun[] = {82, 81, 80, 79, -1};                 // NÜNI
static int WordStundeZehn[] = {90, 91, 92, 93, -1};                 // ZÄNI
static int WordStundeElf[] = {95, 96, 97, 98, -1};                  // EUFI
static int WordStundeZwoelf[] = {106, 105, 104, 103, 102, 101, -1}; // ZWÖUFI

static int *WordStunden[] = {WordStundeZwoelf, WordStundeEins, WordStundeZwei, WordStundeDrei, WordStundeVier,
                             WordStundeFuenf, WordStundeSechs, WordStundeSieben, WordStundeAcht, WordStundeNeun,
                             WordStundeZehn, WordStundeElf, WordStundeZwoelf
};

// Minute
static int WordMinFuenf[] = {8, 9, 10, -1};                   // FÜF
static int WordMinZehn[] = {14, 13, 12, -1};                  // ZÄÄ
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

// Tetris variables
uint8_t board[11][11] = {0}; // 0 = empty, >0 = color index
int tetrisDir = 0; // 1=rotate, 2=right, 3=down, 4=left, 5=new game, 6=exit game
int tetrisScore = 0;
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
    {1,0,0,0},
    {1,1,1,0},
    {0,0,0,0},
    {0,0,0,0}
  },
  // L
  {
    {0,0,1,0},
    {1,1,1,0},
    {0,0,0,0},
    {0,0,0,0}
  },
  // O
  {
    {1,1,0,0},
    {1,1,0,0},
    {0,0,0,0},
    {0,0,0,0}
  },
  // S
  {
    {0,1,1,0},
    {1,1,0,0},
    {0,0,0,0},
    {0,0,0,0}
  },
  // T
  {
    {0,1,0,0},
    {1,1,1,0},
    {0,0,0,0},
    {0,0,0,0}
  },
  // Z
  {
    {1,1,0,0},
    {0,1,1,0},
    {0,0,0,0},
    {0,0,0,0}
  }
};

// Current piece state
int currentTetromino, rotation, posX, posY;
uint8_t currentPiece[4][4]; // Store current piece with rotation applied
bool gameOver = false;

// Map (x, y) to LED index for serpentine wiring
int xyToIndex(int x, int y) {
  if (y % 2 == 0) {
    return y * 11 + x;
  } else {
    return y * 11 + (11 - 1 - x);
  }
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
char wordGuessrLettersCopy[121];

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
int wordGuessrActiveWordIndex[20] = {}; // max 20 letters
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
 * Snake: places a snack on an empty space in snake game
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

// Tetris: Spawn a new tetromino at the top
void spawnTetromino() {
  currentTetromino = random(0, 7);
  rotation = 0;
  posX = 3; // Centered
  posY = -1;  // Spawn one line above visible area
  // Copy initial tetromino to current piece
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      currentPiece[i][j] = tetrominos[currentTetromino][i][j];
    }
  }
  lastDrop = millis();  // Reset timer so piece has time to display before first drop
}

// Tetris: Check collision for current piece at (x, y) with rotation
bool checkCollision(int x, int y, int rot) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
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
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
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
  for (int y = 0; y < 11; y++) {
    bool full = true;
    for (int x = 0; x < 11; x++) {
      if (!board[y][x]) {
        full = false;
        break;
      }
    }
    if (full) {
      // Animate line
      for (int t = 0; t < 3; t++) {
        for (int x = 0; x < 11; x++) {
          pixels.setPixelColor(xyToIndex(x, y), White);
        }
        pixels.show();
        delay(80);
        for (int x = 0; x < 11; x++) {
          pixels.setPixelColor(xyToIndex(x, y), 0);
        }
        pixels.show();
        delay(80);
      }
      // Remove line and shift down
      for (int yy = y; yy > 0; yy--) {
        for (int x = 0; x < 11; x++) {
          board[yy][x] = board[yy-1][x];
        }
      }
      for (int x = 0; x < 11; x++) board[0][x] = 0;
      tetrisScore += 10;
    }
  }
}

// Tetris: Draw the board and current piece
void drawBoard() {
  pixels.clear();
  // Draw placed blocks
  for (int y = 0; y < 11; y++) {
    for (int x = 0; x < 11; x++) {
      if (board[y][x]) {
        pixels.setPixelColor(xyToIndex(x, y), GameColors[board[y][x]-1]);
      }
    }
  }
  // Draw current piece
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
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
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      rotated[j][3-i] = currentPiece[i][j];
    }
  }
  // Check collision for rotated piece
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (rotated[i][j]) {
        int nx = posX + j;
        int ny = posY + i;
        if (nx < 0 || nx >= 11 || ny >= 11 || board[ny][nx]) {
          return; // Collision, do not rotate
        }
      }
    }
  }
  // Apply rotation
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
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

// Tetris: Handle left movement
void handleLeft() {
  if (!checkCollision(posX - 1, posY, rotation)) {
    posX--;
    drawBoard();
  }
}

// Tetris: Handle right movement
void handleRight() {
  if (!checkCollision(posX + 1, posY, rotation)) {
    posX++;
    drawBoard();
  }
}

// Tetris: Handle rotation
void handleRotate() {
  rotateTetromino();
}

// Tetris: Handle down movement
void handleDown() {
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


/*
 * Wordguessr: find a random index of a letter in the wordGuessrLetters, return -1 if letter is not in the word
 * @param letter the letter to find
 * @param allLetters the string to search in
 */
int wordGuessrFindIndex(char letter) {
  int indices[16]; // max 16 occurences of a letter (i has 15)
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
        //Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            if (header.indexOf("snake") >= 0) {
              // Client is playing snake game:
              const char *url = header.c_str();
              if (extractParameterValue(url, "dir=") > 0 && extractParameterValue(url, "dir=") < 7) {
                snakePrevDir = snakeDir;
                snakeDir = extractParameterValue(url, "dir=");
              }
              if (!inSnake && snakeDir == 5 && power == 1) {
                // start new snake game
                inSnake = true;
                inMastermind = false;
                inWordGuessr = false;
                inTetris = false;
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
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-type:text/plain"));
              client.println(F("Access-Control-Allow-Origin: *"));
              client.println(F("Connection: close"));
              client.println();
              client.println(snakeLen);
            } else if (header.indexOf("tetris") >= 0) {
              // Client is playing Tetris game:
              const char *url = header.c_str();
              if (extractParameterValue(url, "dir=") > 0 && extractParameterValue(url, "dir=") < 7) {
                tetrisDir = extractParameterValue(url, "dir=");
              }
              // int tetrisDir = 0; // 1=rotate, 2=right, 3=down, 4=left, 5=new game, 6=exit game
              if (!inTetris && tetrisDir == 5 && power == 1) {
                // start new Tetris game
                inTetris = true;
                inMastermind = false;
                inWordGuessr = false;
                inSnake = false;
                blank();
                pixels.show();
                handleRestart();
              } else if (inTetris && tetrisDir == 1) {
                handleRotate();
              } else if (inTetris && tetrisDir == 2) {
                handleRight();
              } else if (inTetris && tetrisDir == 3) {
                handleDown();
              } else if (inTetris && tetrisDir == 4) {
                handleLeft();
              } else if (inTetris && tetrisDir == 6) {
                // exit current Tetris game
                inTetris = false;
                lastMinuteWordClock = 61;
              }
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-type:text/plain"));
              client.println(F("Access-Control-Allow-Origin: *"));
              client.println(F("Connection: close"));
              client.println();
              client.println(tetrisScore);
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
              client.println(F("<!doctype html><html lang='en'><head><meta charset='utf-8'><title>grrd s WordClock</title><link id='icon' rel='icon' href='data:image/png;base64,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'><meta name='description' content='grrd s WordClock is a web WordClock and a user interface for the Wemos Mini D1 Lite Clock'><meta name='viewport' content='width=device-width,initial-scale=1'><meta name='theme-color' content='#444'><meta name='apple-mobile-web-app-title' content='WordClock'><link id='iphone' rel='apple-touch-icon'><meta name='apple-mobile-web-app-capable' content='yes'><meta name='apple-mobile-web-app-status-bar-style' content='black'><style>:root{--main-color:#878ade}html{height:100%;user-select:none}body{background:linear-gradient(#444,#222);min-width:100vw;margin:0;position:fixed;overflow:hidden;font-family:Arial,sans-serif;font-size:large;color:#fff;text-shadow:1px 1px 2px #000;height:100%}.p{width:100vw;position:fixed;top:0;left:0;right:0;bottom:0;background:linear-gradient(#444,#222)}#c,#ctrl,.t{font:6px sans-serif;fill:#555;text-shadow:none;text-anchor:middle;width:100vmin;margin:auto;display:block}@media (orientation:landscape){#ctrl{width:100%;max-width:60vh}}@media (orientation:portrait){#ctrl{width:100%}}.t,.w100{width:100%}.M1,.M2,.M3,.M4{font-size:8px}.H0 .H0,.H1 .H1,.H10 .H10,.H11 .H11,.H2 .H2,.H3 .H3,.H4 .H4,.H5 .H5,.H6 .H6,.H7 .H7,.H8 .H8,.H9 .H9,.M1 .M1,.M10 .M10,.M15 .M15,.M2 .M1,.M2 .M2,.M20 .M20,.M3 .M1,.M3 .M2,.M3 .M3,.M30 .M30,.M4 .M1,.M4 .M2,.M4 .M3,.M4 .M4,.M5 .M5,.MA .MA,.MV .MV,.g{fill:var(--main-color);text-shadow:0 0 10px var(--main-color)}.off .g:not(.colorBtn){fill:#555;text-shadow:none}.d .H0 .H0,.d .H1 .H1,.d .H10 .H10,.d .H11 .H11,.d .H2 .H2,.d .H3 .H3,.d .H4 .H4,.d .H5 .H5,.d .H6 .H6,.d .H7 .H7,.d .H8 .H8,.d .H9 .H9,.d .M1 .M1,.d .M10 .M10,.d .M15 .M15,.d .M2 .M1,.d .M2 .M2,.d .M20 .M20,.d .M3 .M1,.d .M3 .M2,.d .M3 .M3,.d .M30 .M30,.d .M4 .M1,.d .M4 .M2,.d .M4 .M3,.d .M4 .M4,.d .M5 .M5,.d .MA .MA,.d .MV .MV,.d .g{filter:brightness(70%)}a:link{color:var(--main-color)}a:visited{color:var(--main-color);filter:brightness(85%)}a:focus,a:hover{color:var(--main-color);filter:brightness(125%)}a:active{color:var(--main-color);filter:brightness(125%)}#s,#xMM,#xS,#xSN,#xTE,#xWG{position:absolute;right:4vmin;bottom:4vmin}#p{position:absolute;left:4vmin;bottom:4vmin}#sMM,#sSN,#sTE,#sWG{position:absolute;left:4vmin;bottom:4vmin;display:flex;align-items:center}.sb,.snb,.svgMsg{width:4.5vmin;height:4.5vmin;min-width:30px;min-height:30px;stroke:#555;stroke-linejoin:round;stroke-linecap:round;stroke-width:6;fill:none;z-index:1000}circle{pointer-events:none}input[type=text]{width:calc(100% - 4.5vmin - 40px);border:2px solid #555;border-radius:5px;background-color:transparent;color:#fff;padding:10px;font-size:larger}input[type=text]:focus{border:2px solid #fff;outline:0}input[type=text].error{border:2px solid #f70562}input[type=text].ok{border:2px solid #059c7d}.sb.g,.sb:hover,.snb.g,.snb:hover{stroke:#fff;text-shadow:0 0 10px #fff;cursor:pointer}path.snb{stroke-width:1.4;fill:#333;fill-opacity:0.01}#pMM,#pS,#pSN,#pTE,#pWG{transform:translateX(100vw);visibility:hidden;opacity:0}.pC{display:block;position:absolute;overflow:auto;top:0;left:0;right:0;margin:0 auto 0 auto;width:600px;max-width:calc(100vw - 40px);height:100%}.c,.c>span,.pb{display:flex;justify-content:space-between;margin-bottom:20px;align-items:center;flex-wrap:wrap}.pf{margin-top:60px}#co{appearance:none;background-color:transparent;width:4.5vmin;height:4.5vmin;min-width:30px;min-height:30px;border:none;cursor:pointer}#co::-webkit-color-swatch{border-radius:50%;border:.45vmin solid #555}#co::-moz-color-swatch{border-radius:50%;border:.45vmin solid #555}#co::-webkit-color-swatch:hover{border:.45vmin solid #fff}#co::-moz-color-swatch:hover{border:.45vmin solid #fff}.h{display:none}svg[data-num='1']{fill:#fc034e}[data-num='2']{fill:#fc6f03}[data-num='3']{fill:#fcce03}[data-num='4']{fill:#18fc03}[data-num='5']{fill:#0384fc}[data-num='6']{fill:#f803fc}.si{animation-name:si;animation-fill-mode:forwards;animation-duration:.7s}@keyframes si{0%{transform:translateX(100vw);visibility:hidden;opacity:0}1%{transform:translateX(100vw);visibility:visible;opacity:1}100%{transform:translateX(0);visibility:visible;opacity:1}}.so{animation-name:so;animation-fill-mode:forwards;animation-duration:.7s}@keyframes so{0%{transform:translateX(0);visibility:visible;opacity:1}99%{transform:translateX(-100vw);visibility:visible;opacity:1}100%{transform:translateX(-100vw);visibility:hidden;opacity:0}}.sil{animation-name:sil;animation-fill-mode:forwards;animation-duration:.7s}@keyframes sil{0%{transform:translateX(0);visibility:visible;opacity:1}99%{transform:translateX(100vw);visibility:visible;opacity:1}100%{transform:translateX(100vw);visibility:hidden;opacity:0}}.sor{animation-name:sor;animation-fill-mode:forwards;animation-duration:.7s}@keyframes sor{0%{transform:translateX(-100vw);visibility:hidden;opacity:0}1%{transform:translateX(-100vw);visibility:visible;opacity:1}100%{transform:translateX(0);visibility:visible;opacity:1}}.sl{appearance:none;width:100%;height:4px;border-radius:2px;background:0 0;margin:10px 0;direction:rtl;border:solid calc(2px + .2vmin) #555}.sl::-webkit-slider-thumb{appearance:none;width:3vmin;height:3vmin;min-width:20px;min-height:20px;border-radius:50%;background:var(--main-color);cursor:pointer;outline:solid .45vmin #555}.sl::-webkit-slider-thumb:hover{outline:solid .45vmin #fff}.sl::-moz-range-thumb{width:3vmin;height:3vmin;min-width:20px;min-height:20px;border-radius:50%;background:var(--main-color);cursor:pointer;outline:solid .45vmin #555}.sl::-moz-range-thumb:hover{outline:solid .45vmin #fff}</style></head><body><div id='pC' class='p'><svg id='c' viewBox='0 0 115 110' preserveAspectRatio='xMidYMid slice' role='img'><g stroke='#555' fill='none' stroke-width='0.7'><path d='M 106 109.8 Q 106 106.4 109.4 106.4'/><path d='M 107.2 109.8 Q 107.2 107.8 109.4 107.8'/></g><circle cx='108.8' cy='109.5' r='0.6' fill='#555'/></svg> <svg id='p' class='sb' viewBox='0 0 74 74'><line x1='37' y1='15' x2='37' y2='27'/><circle cx='37' cy='37' r='33'/><path d='M 48 22 A 18 18 0 1 1 26 22'/></svg> <svg id='s' class='sb' viewBox='0 0 74 74'><path d='M30 3 A 37 37 0 0 1 44 3 L 44 13 A 25 25 0 0 1 54.5 20 L 63 14 A 37 37 0 0 1 70 25.5 L 61 31 A 25 25 0 0 1 61 42.5 L 70 48.5 A 37 37 0 0 1 63 60 L 54.5 54 A 25 25 0 0 1 44 61 L 44 71 A 37 37 0 0 1 30 71 L 30 61 A 25 25 0 0 1 19.5 54 L 11 60 A 37 37 0 0 1 4 48.5 L 13 42.5 A 25 25 0 0 1 13 31 L 4 25.5 A 37 37 0 0 1 11 14 L 19.5 20 A 25 25 0 0 1 30 13 Z'/><circle cx='37' cy='37' r='12'/></svg></div><div id='pS' class='p'><div class='pC'><div><svg class='t' viewBox='5 0 105 25' preserveAspectRatio='xMidYMid slice' role='img'/></div><div class='pb'><label for='co'>Weli Farb wosch?</label> <input type='color' id='co' value='#ffffff'/></div><div class='pb'><label>Cha mi nid entscheide. Chli vo auem.</label> <svg id='rm' class='sb' viewBox='0 0 70 70'><path class='n'/><path class='h y'/></svg></div><div class='pb'><label class='w100' for='speed'>Wie schnäu?</label> <input type='range' id='speed' class='sl' min='50' max='2000'/> <label>gmüetlech</label> <label>jufle</label></div><div class='pb'><label>Ir Nacht chli weniger häu.</label> <svg id='dm' class='sb' viewBox='0 0 70 70'><path class='h n'/><path class='y'/></svg></div><div class='pb'><label>I ha ke Angst vor Gspängster.</label> <svg id='gm' class='sb' viewBox='0 0 70 70'><path class='h n'/><path class='y'/></svg></div><div class='pb'><label>Schnäu e Rundi Snake spile.</label> <svg id='SN' class='sb play' viewBox='-2 -1 12 16'/></div><div class='pb'><label>Chli Tetris zocke.</label> <svg id='TE' class='sb play' viewBox='-2 -1 12 16'/></div><div class='pb'><label>Oder hurti es Mastermind.</label> <svg id='MM' class='sb play' viewBox='-2 -1 12 16'/></div><div class='pb'><label>Es paar Wörtli errate.</label> <svg id='WG' class='sb play' viewBox='-2 -1 12 16'/></div><div class='pf'><p class='c'>Handgmachti Software us Bärn</p><p class='c'><span>Gérard&nbsp;Tyedmers</span> <span><svg width='24' height='24' viewBox='0 -5 160 170' stroke='#fff' fill='none' stroke-width='10'><circle cx='80' cy='80' r='70'/><path d='M27 32c7 20 93 43 121 28M13 60c-3 30 117 60 135 35M16 106c16 19 84 39 112 24M100 13C34 3 10 130 65 148M100 13C70 33 45 118 65 148M100 13c13 22 5 112-35 135M100 13c60 35 20 147-35 135'/></svg>&nbsp; <a href='https://grrd.ch'>grrd.ch</a> </span><span><svg width='30px' height='24px' viewBox='0 0 222 179' stroke='#fff' fill='none' stroke-width='10' stroke-linecap='round'><g transform='translate(-10,10) rotate(-6)'><rect x='15' y='25' rx='10' ry='10' width='192' height='129'/><path d='M15 40 C131 125, 91 125, 207 40'/><line x1='15' y1='134' x2='77' y2='90'/><line x1='207' y1='134' x2='145' y2='90'/></g></svg>&nbsp; <a href='mailto:grrd@gmx.net'>grrd@gmx.net</a></span></p></div></div><svg id='xS' class='sb' viewBox='0 0 70 70'><path class='y'/></svg></div><div id='pSN' class='p'><div class='pC'><div><svg class='t' viewBox='5 0 105 25' preserveAspectRatio='xMidYMid slice' role='img'></svg></div><svg id='ctrl' viewBox='0 0 115 110' preserveAspectRatio='xMidYMid slice' role='img'><path data-num='1' class='snb' transform='scale(3.8) translate(8,10) rotate(270)'/><path data-num='2' class='snb' transform='scale(3.8) translate(20,7.5)'/><path data-num='3' class='snb' transform='scale(3.8) translate(22,19) rotate(90)'/><path data-num='4' class='snb' transform='scale(3.8) translate(10,21.5) rotate(180)'/></svg></div><svg id='xSN' class='sb' viewBox='0 0 70 70'><path class='n'/></svg> <span id='sSN'></span></div><div id='pTE' class='p'><div class='pC'><div><svg class='t' viewBox='5 0 105 25' preserveAspectRatio='xMidYMid slice' role='img'></svg></div><svg id='ctrlt' viewBox='0 -15 115 130' preserveAspectRatio='xMidYMid slice' role='img'><g class='snb' stroke-width='6' data-num='1' transform='scale(0.8) translate(34,-15)'><path data-num='1' stroke='none' fill='#333' fill-opacity='0.01' d='M13 37 A 24 24 0 1 1 13 37.5'></path><path data-num='1' id='arc' d='M23 45 L17.5 50.5 A 24 24 10 0 1 34 13 L34 4 L50 17 L34 33 L34 24 '></path><use data-num='1' href='#arc' transform='rotate(180 37 37)'></use></g><path data-num='2' class='snb' transform='scale(3.8) translate(20,7.5)'/><path data-num='3' class='snb' transform='scale(3.8) translate(22,19) rotate(90)'/><path data-num='4' class='snb' transform='scale(3.8) translate(10,21.5) rotate(180)'/></svg></div><svg id='xTE' class='sb' viewBox='0 0 70 70'><path class='n'/></svg> <span id='sTE'></span></div><div id='pMM' class='p'><div class='pC'><div><svg class='t' viewBox='5 0 105 25' preserveAspectRatio='xMidYMid slice' role='img'></svg></div><label class='c'>Hie chasch d Farb useläse:</label> <span class='c'><svg class='sb cb' data-num='1'/><svg class='sb cb' data-num='2'/><svg class='sb cb' data-num='3'/><svg class='sb cb' data-num='4'/><svg class='sb cb' data-num='5'/><svg class='sb cb' data-num='6'/></span><label class='c'>Hie muesch di Versuech iigäh:</label> <span class='c'><svg class='sb cdb'/><svg class='sb cdb'/><svg class='sb cdb'/><svg class='sb cdb'/><svg id='cMM' class='sb' viewBox='0 0 70 70'><path class='y'/></svg></span></div><svg id='xMM' class='sb' viewBox='0 0 70 70'><path class='n'/></svg> <span id='sMM'/></div><div id='pWG' class='p'><div class='pC'><div><svg class='t' viewBox='4 0 106 25' preserveAspectRatio='xMidYMid slice' role='img'/></div><label for='wi' class='c'><br>Weles isch ds gsuechte Wort:</label> <span class='c'><input type='text' id='wi' maxlength='20' oninput='this.value = this.value.toUpperCase().replace(/[^A-ZÄÖÜ]/g, &#39;&#39;)' spellcheck='false' autocomplete='off'> <svg id='cWG' class='sb' viewBox='0 0 70 70'><path class='y'/></svg></span></div><svg id='xWG' class='sb' viewBox='0 0 70 70'><path class='n'/></svg> <span id='sWG'/></div><script>!function(){'use strict';const e=document,t=C('c'),n=C('pC'),r=C('pS'),a=C('pSN'),o=C('pTE'),c=C('pMM'),s=C('pWG'),i=C('co'),u=C('speed'),d=C('wi'),f=C('rm'),l=C('gm'),M=C('dm'),g=e.getElementsByTagName('body')[0],H=W('cdb'),m=W('cb'),w='click',h='<svg class=\"svgMsg\" viewBox=\"0 0 70 70\"> <circle cx=\"35\" cy=\"35\" r=\"25\" fill=';let p,v,S,L=1,E=1,y=1,T=0,b=255,A=0,I=0,R=0,x=0,_=0,G='1',N=0,k=0,D=0;function C(t){return e.getElementById(t)}function W(t){return e.getElementsByClassName(t)}function Z(e){return localStorage.getItem(e)}function B(e,t){return localStorage.setItem(e,t)}function V(e){return e.classList}function O(e){return e.children}function F(e,t,n){e.setAttribute(t,n)}function X(e,t,n){e.addEventListener(t,n)}function q(){p=new Date,E&&(p.getHours()>=22||p.getHours()<7)?V(g).add('d'):V(g).remove('d'),S!==p.getMinutes()&&(S=p.getMinutes(),V(t).remove(...V(t)),0!==L&&(S>=55?V(t).add('M5','MV'):S>=50?V(t).add('M10','MV'):S>=45?V(t).add('M15','MV'):S>=40?V(t).add('M20','MV'):S>=35?V(t).add('M5','MA','M30'):S>=30?V(t).add('M30'):S>=25?V(t).add('M5','MV','M30'):S>=20?V(t).add('M20','MA'):S>=15?V(t).add('M15','MA'):S>=10?V(t).add('M10','MA'):S>=5&&V(t).add('M5','MA'),v=p.getHours(),S>=25&&(v+=1),v%=12,V(t).add('H'+v.toString()),V(t).add('M'+(S%5).toString())))}function U(){T&&(b&&!I?(b-=1,A+=1):A?(A-=1,I+=1):(I-=1,b+=1),z('rgb('+b+', '+A+', '+I+')'),setTimeout(U,u.value/10))}function j(e){L=e,L?V(g).remove('off'):V(g).add('off'),S=-1,q()}function K(t,n){e.activeElement.blur(),V(t).remove('sor'),V(n).remove('sil'),V(t).add('so'),V(n).add('si')}function J(e,t){V(e).remove('so'),V(t).remove('si'),V(e).add('sor'),V(t).add('sil')}function z(t){e.documentElement.style.setProperty('--main-color',t)}function P(e){e!==T&&(V(O(f)[0]).toggle('h'),V(O(f)[1]).toggle('h')),T=e,T?U():z(i.value)}function Q(e){e!==y&&(V(O(l)[0]).toggle('h'),V(O(l)[1]).toggle('h')),y=e}function Y(e){e!==E&&(V(O(M)[0]).toggle('h'),V(O(M)[1]).toggle('h')),E=e}function $(){let e=parseInt(i.value.substring(1,3),16),t=parseInt(i.value.substring(3,5),16),n=parseInt(i.value.substring(5,7),16);B('wc_c',i.value),B('wc_r',T),B('wc_d',E),B('wc_g',y),B('wc_s',u.value.toString());let r=new XMLHttpRequest;r.open('GET','/update_params?red='+e+'&green='+t+'&blue='+n+'&rainbow='+T+'&darkmode='+E+'&speed='+u.value+'&power='+L+'&ghost='+y,!0),r.send()}function ee(e){let t=new XMLHttpRequest;t.onreadystatechange=function(){4===this.readyState&&200===this.status&&(R=10*(parseInt(t.responseText)-3),R>x&&(x=R,B('wc_sc',x)),C('sSN').innerHTML='Score: '+R+' / High-Score : '+x)},t.open('GET','snake?dir='+e,!0),t.send()}function te(e){let t=new XMLHttpRequest;t.onreadystatechange=function(){4===this.readyState&&200===this.status&&(R=parseInt(t.responseText),R>_&&(_=R,B('wc_te',_)),C('sTE').innerHTML='Score: '+R+' / High-Score : '+_)},t.open('GET','tetris?dir='+e,!0),t.send()}function ne(t){let n='';if(1===t)n='mastermind?c4=0',re(),ae();else if(2===t)n='mastermind?c4=7',re();else{if(e.querySelectorAll('[data-num=\"1\"], [data-num=\"2\"], [data-num=\"3\"], [data-num=\"4\"], [data-num=\"5\"], [data-num=\"6\"]').length<14)return void ae('Muesch zersch aues uswähle.');n='mastermind?c1='+H[0].getAttribute('data-num')+'&c2='+H[1].getAttribute('data-num')+'&c3='+H[2].getAttribute('data-num')+'&c4='+H[3].getAttribute('data-num'),re()}let r=new XMLHttpRequest;r.onreadystatechange=function(){if(4===this.readyState&&200===this.status){let e=JSON.parse(r.responseText);N=e.place,k=e.try,4===N?ae('Bravo! I '+k+' Mau usegfunde.'):11===k?ae('Schad, jetz hesch verlore.'):ae()}},r.open('GET',n,!0),r.send()}function re(){Array.from(H).forEach(function(e){F(e,'data-num','')})}function ae(e){C('sMM').innerHTML=e||h+'\"white\"/></svg>&nbsp;am richtige Ort&nbsp;'+h+'\"cornflowerblue\"/></svg>&nbsp;di richtigi Farb'}function oe(e){let t;t='1'===e?'wordguessr?new':'2'===e?'wordguessr?exit':'wordguessr?word='+d.value;let n=new XMLHttpRequest;n.onreadystatechange=function(){if(4===this.readyState&&200===this.status){let e=JSON.parse(n.responseText);0===e.score?(V(d).add('error'),setTimeout(function(){V(d).remove('error'),d.value=''},100)):1===e.score&&(D+=e.score,C('sWG').innerHTML=D+' hesch usegfunde.',V(d).add('ok'),setTimeout(function(){V(d).remove('ok'),d.value=''},100))}},n.open('GET',t,!0),n.send()}setInterval(q,100),X(C('p'),w,function(){j(1-L),$()}),X(C('s'),w,function(){K(n,r)}),X(C('xS'),w,function(){J(n,r),V(r).remove('sor'),$()}),X(C('SN'),w,function(){K(r,a),ee(5)}),X(C('xSN'),w,function(){J(r,a),ee(6)}),X(C('TE'),w,function(){K(r,o),te(5)}),X(C('xTE'),w,function(){J(r,o),te(6)}),X(C('MM'),w,function(){K(r,c),ne(1)}),X(C('xMM'),w,function(){J(r,c),ne(2)}),X(C('cMM'),w,ne),X(C('WG'),w,function(){K(r,s),oe('1'),D=0,C('sWG').innerHTML=''}),X(C('xWG'),w,function(){J(r,s),oe('2')}),X(C('cWG'),w,oe),Array.from(W('snb')).forEach(function(e,t){F(e,'d','M2 2 L9 7 L2 12 Z'),X(e,w,t<4?function(e){ee(e.target.getAttribute('data-num'))}:function(e){te(e.target.getAttribute('data-num'))})}),Array.from(W('n')).forEach(function(e){F(e,'d','M10 20 L20 10 L35 25 L50 10 L60 20 L45 35 L60 50 L50 60 L35 45 L20 60 L10 50 L25 35 L10 20 Z'),F(e,'transform','scale(0.9) translate(5,5)')}),Array.from(W('y')).forEach(function(e){F(e,'d','M0 40 L10 30 L20 40 L50 10 L60 20 L20 60 L0 40 Z'),F(e,'transform','scale(0.85) translate(5,5)')}),Array.from(W('play')).forEach(function(e){e.innerHTML='<path d=\"M2 2 L9 7 L2 12 Z\" stroke-width=\"1.4\"/>'}),Array.from(m).forEach(function(e){X(e,w,function(e){Array.from(m).forEach(function(e){V(e).remove('g')}),V(e.target).add('g'),G=e.target.getAttribute('data-num')}),e.innerHTML='<circle cx=\"35\" cy=\"35\" r=\"25\"/>',F(e,'viewBox','0 0 70 70')}),Array.from(H).forEach(function(e){X(e,w,function(e){F(e.target,'data-num',G),ae()}),e.innerHTML='<circle cx=\"35\" cy=\"35\" r=\"25\"/>',F(e,'viewBox','0 0 70 70')}),e.onkeydown=function(e){let t=0;switch(e.key){case'ArrowUp':t=1;break;case'ArrowRight':t=2;break;case'ArrowDown':t=3;break;case'ArrowLeft':t=4;break;case'Enter':V(s).contains('si')&&oe()}t&&V(a).contains('si')&&(ee(t),V(O(C('ctrl'))[t-1]).add('g'),setTimeout(function(){V(O(C('ctrl'))[t-1]).remove('g')},200)),t&&V(o).contains('si')&&(te(t),V(O(C('ctrlt'))[t-1]).add('g'),setTimeout(function(){V(O(C('ctrlt'))[t-1]).remove('g')},200))},X(i,'change',e=>{z(i.value)}),X(f,w,e=>{P(1-T)}),X(l,w,e=>{Q(1-y)}),X(M,w,e=>{Y(1-E)}),Z('wc_c')&&(i.value=Z('wc_c'),z(i.value)),Z('wc_r')&&P(parseInt(Z('wc_r'))),Z('wc_g')&&Q(parseInt(Z('wc_g'))),Z('wc_d')&&Y(parseInt(Z('wc_d'))),Z('wc_s')&&(u.value=parseInt(Z('wc_s'))),Z('wc_sc')&&(x=Z('wc_sc')),Z('wc_te')&&(_=Z('wc_te')),C('iphone').href=C('icon').href,'E g,S g,D,I g,S g,C g,H g,W,F M5,Ü M5,F M5,V M15,I M15,E M15,R M15,T M15,U M15,T,Z M10,Ä M10,Ä M10,Y,Z M20,W M20,Ä M20,N M20,Z M20,G M20,Q,D,V MV,O MV,R MV,K,A MA,B MA,D,H M30,A M30,U M30,B M30,I M30,T,Z,E H1,I H1,S H1,Q,Z H2,W H2,Ö H2,I H2,D H3,R H3,Ü H3,Z,V H4,I H4,E H4,R H4,I H4,F H5,Ü H5,F H5,I H5,T,G,M,S H6,Ä H6,C H6,H H6,S H6 H7,I H6 H7,B H7,N H7,I H7,A H8,C H8,H H8,T H8,I H8,N H9,Ü H9,N H9,I H9,O,F,C,D,Z H10,Ä H10,N H10,I H10,X,E H11,U H11,F H11,I H11,O,K,G,Z H0,W H0,Ö H0,U H0,F H0,I H0,L,X,L,Y,B,° M1,° M2,P,° M3,° M4,M,K'.split(',').forEach(function(n,r){const a=e.createElementNS('http://www.w3.org/2000/svg','text');F(a,'x',r%11*10+7),F(a,'y',10*Math.ceil((r+1)/11)),[113,114,116,117].includes(r)&&F(a,'y',112.5),n.split(' ').forEach(function(e,t){t?V(a).add(e):a.textContent=e}),t.appendChild(a)});const ce=['ewfGRRDcSajnWORDuCLOCK','ewfGRRDcSajmSNAKExlbdk','ewfGRRDcSajmTETRISlbdk','ewfGRRDcSajMASTERMINDk','ewfGRRDcSajWORDbGUESSR'];Array.from(W('t')).forEach(function(t,n){for(let r=0;r<22;r++){const a=e.createElementNS('http://www.w3.org/2000/svg','text');let o=ce[n].substring(r,r+1),c=o.toUpperCase();F(a,'x',r%11*10+7),F(a,'y',10*Math.ceil((r+1)/11)),a.textContent=c,o===c&&F(a,'class','g'),t.appendChild(a)}});let se=new XMLHttpRequest;se.onreadystatechange=function(){if(4===this.readyState&&200===this.status){let r=JSON.parse(se.responseText);r.rainbow||(i.value=(e=r.red,t=r.green,n=r.blue,'#'+(1<<24|e<<16|t<<8|n).toString(16).slice(1))),z(i.value),Y(r.darkmode),P(r.rainbow),Q(r.ghost),j(r.power),u.value=r.speed}var e,t,n},se.open('GET','get_params',!0),se.send()}();</script></body></html>"));
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
        // move snake one step forward
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
      chase(Red);
      inTetris = false;
      lastMinuteWordClock = 61;
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
