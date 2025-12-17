// Tetris for ESP8266 with Adafruit NeoPixel 11x11 serpentine matrix
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiManager.h>
#include <WebSocketsServer.h>

#define LED_PIN    D7
#define LED_COUNT  121
#define MATRIX_WIDTH  11
#define MATRIX_HEIGHT 11

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(80);
WiFiManager wifiManager;
WebSocketsServer webSocket = WebSocketsServer(81);

// forward declarations
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

// Game variables
uint8_t board[MATRIX_HEIGHT][MATRIX_WIDTH] = {0}; // 0 = empty, >0 = color index
int score = 0;

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

// Tetromino colors (GRB)
const uint32_t tetrominoColors[7] = {
  strip.Color(0,255,255), // I - cyan
  strip.Color(0,0,255),   // J - blue
  strip.Color(255,165,0), // L - orange
  strip.Color(255,255,0), // O - yellow
  strip.Color(0,255,0),   // S - green
  strip.Color(128,0,128), // T - purple
  strip.Color(255,0,0)    // Z - red
};

// Current piece state
int currentTetromino, rotation, posX, posY;
uint8_t currentPiece[4][4]; // Store current piece with rotation applied
bool gameOver = false;
unsigned long lastDrop = 0;
const unsigned long dropInterval = 600; // ms

// Map (x, y) to LED index for serpentine wiring
int xyToIndex(int x, int y) {
  if (y % 2 == 0) {
    return y * MATRIX_WIDTH + x;
  } else {
    return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\nStarting Tetris...");
  
  // WiFiManager initialization
  wifiManager.autoConnect("TetrisClock");
  
  Serial.println("WiFi connected!");
  IPAddress ip = WiFi.localIP();
  Serial.print("Local IP address: ");
  Serial.println(ip);
  
  strip.begin();
  strip.show();
  server.on("/", handleRoot);
  server.on("/restart", handleRestart);
  server.begin();
  spawnTetromino();
  drawBoard();  // Draw the initial piece before game loop starts
  // Start WebSocket server for low-latency controls and score updates
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

// Send current score to all connected WebSocket clients
void sendScoreToClients() {
  String msg = "score:" + String(score);
  webSocket.broadcastTXT(msg);
}

// Spawn a new tetromino at the top
void spawnTetromino() {
  currentTetromino = random(0, 7);
  rotation = 0;
  posX = 3; // Centered
  posY = -2;  // Spawn one line above visible area
  // Copy initial tetromino to current piece
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      currentPiece[i][j] = tetrominos[currentTetromino][i][j];
    }
  }
  lastDrop = millis();  // Reset timer so piece has time to display before first drop
}

// Check collision for current piece at (x, y) with rotation
bool checkCollision(int x, int y, int rot) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (currentPiece[i][j]) {
        int nx = x + j;
        int ny = y + i;
        // Only check collisions with boundaries and board for visible area
        if (nx < 0 || nx >= MATRIX_WIDTH) return true;
        if (ny >= MATRIX_HEIGHT) return true;  // Piece hit bottom
        if (ny >= 0 && board[ny][nx]) return true;  // Only check board collision if visible
      }
    }
  }
  return false;
}

// Place current piece on the board
void placeTetromino() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (currentPiece[i][j]) {
        int nx = posX + j;
        int ny = posY + i;
        if (nx >= 0 && nx < MATRIX_WIDTH && ny >= 0 && ny < MATRIX_HEIGHT) {
          board[ny][nx] = currentTetromino + 1;
        }
      }
    }
  }
}

// Clear full lines and animate
void clearLines() {
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    bool full = true;
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      if (!board[y][x]) {
        full = false;
        break;
      }
    }
    if (full) {
      // Animate line
      for (int t = 0; t < 3; t++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
          strip.setPixelColor(xyToIndex(x, y), strip.Color(255,255,255));
        }
        strip.show();
        delay(80);
        for (int x = 0; x < MATRIX_WIDTH; x++) {
          strip.setPixelColor(xyToIndex(x, y), 0);
        }
        strip.show();
        delay(80);
      }
      // Remove line and shift down
      for (int yy = y; yy > 0; yy--) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
          board[yy][x] = board[yy-1][x];
        }
      }
      for (int x = 0; x < MATRIX_WIDTH; x++) board[0][x] = 0;
      score += 10;
      sendScoreToClients();
    }
  }
}

// Draw the board and current piece
void drawBoard() {
  strip.clear();
  // Draw placed blocks
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      if (board[y][x]) {
        strip.setPixelColor(xyToIndex(x, y), tetrominoColors[board[y][x]-1]);
      }
    }
  }
  // Draw current piece
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (currentPiece[i][j]) {
        int nx = posX + j;
        int ny = posY + i;
        if (nx >= 0 && nx < MATRIX_WIDTH && ny >= 0 && ny < MATRIX_HEIGHT) {
          strip.setPixelColor(xyToIndex(nx, ny), tetrominoColors[currentTetromino]);
        }
      }
    }
  }
  strip.show();
}

// Rotate tetromino (clockwise)
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
        if (nx < 0 || nx >= MATRIX_WIDTH || ny >= MATRIX_HEIGHT || (ny >= 0 && board[ny][nx])) {
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

// WebSocket event handler: receive control commands from client
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_CONNECTED) {
    // send current score on new connection
    sendScoreToClients();
    return;
  }
  if (type != WStype_TEXT) return;
  String msg = String((char*)payload);
  if (msg == "left") {
    if (!checkCollision(posX - 1, posY, rotation)) { posX--; if(!gameOver) drawBoard(); }
  } else if (msg == "right") {
    if (!checkCollision(posX + 1, posY, rotation)) { posX++; if(!gameOver) drawBoard(); }
  } else if (msg == "rotate") {
    rotateTetromino();
  } else if (msg == "down") {
    if (!checkCollision(posX, posY + 1, rotation)) {
      posY++;
    } else {
      if (posY < 0) {
        gameOver = true;
      } else {
        placeTetromino();
        clearLines();
        spawnTetromino();
        if (checkCollision(posX, posY, rotation)) gameOver = true;
      }
    }
    if (!gameOver) drawBoard();
  } else if (msg == "restart") {
    handleRestart();
    sendScoreToClients();
  }
}

// Enhanced root handler with score and controls
void handleRoot() {
  String html = "<html><head><title>Tetris ESP8266</title></head><body>";
  html += "<h1>Tetris for ESP8266</h1>";
  html += "<p>Score: <span id=\"score\">" + String(score) + "</span></p>";
  if (gameOver) {
    html += "<h2>Game Over!</h2>";
    html += "<button onclick=\"sendCmd('restart')\">Restart</button> ";
  } else {
    // keep buttons for mouse/touch users (use sendCmd which prefers WS)
    html += "<div>";
    html += "<button onclick=\"sendCmd('left')\">Left</button> ";
    html += "<button onclick=\"sendCmd('right')\">Right</button> ";
    html += "<button onclick=\"sendCmd('rotate')\">Rotate</button> ";
    html += "<button onclick=\"sendCmd('down')\">Down</button> ";
    html += "<button onclick=\"sendCmd('restart')\">Restart</button> ";
    html += "</div>";
  }

  // WebSocket client + keyboard handling (uses WS when available, else falls back to HTTP)
  html += "<script>";
  html += "var ws = null;";
  html += "try {";
  html += "  ws = new WebSocket('ws://' + location.hostname + ':81/');";
  html += "  ws.onopen = function(){ console.log('ws open'); };";
  html += "  ws.onmessage = function(e) {";
  html += "    if (e.data && e.data.indexOf('score:') === 0) {";
  html += "      document.getElementById('score').innerText = e.data.split(':')[1];";
  html += "    }";
  html += "  };";
  html += "  ws.onclose = function(){ console.log('ws closed'); };";
  html += "  ws.onerror = function(e){ console.log('ws error', e); };";
  html += "} catch(e) { console.log('ws init failed'); }";

  html += "function sendCmd(cmd) {";
  html += "  if (ws && ws.readyState === 1) {";
  html += "    ws.send(cmd);";
  html += "  }";
  html += "}";

  html += "document.addEventListener('keydown', function(e) {";
  html += "  if (e.repeat) return;";
  html += "  switch (e.key) {";
  html += "    case 'ArrowLeft': sendCmd('left'); e.preventDefault(); break;";
  html += "    case 'ArrowRight': sendCmd('right'); e.preventDefault(); break;";
  html += "    case 'ArrowUp': sendCmd('rotate'); e.preventDefault(); break;";
  html += "    case 'ArrowDown': sendCmd('down'); e.preventDefault(); break;";
  html += "  }";
  html += "});";

  html += "</script>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleRestart() {
  memset(board, 0, sizeof(board));
  score = 0;
  gameOver = false;
  spawnTetromino();
  drawBoard();
  sendScoreToClients();
  server.send(200, "text/plain", "OK");
}

void loop() {
  server.handleClient();
  webSocket.loop();
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
}
