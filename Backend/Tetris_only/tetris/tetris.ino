// Tetris for ESP8266 with Adafruit NeoPixel 11x11 serpentine matrix
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN    D4
#define LED_COUNT  121
#define MATRIX_WIDTH  11
#define MATRIX_HEIGHT 11

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(80);

// WiFi credentials
const char* ssid = "TetrisClock";
const char* password = "tetris123";

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

// Map (x, y) to LED index for serpentine wiring
int xyToIndex(int x, int y) {
  if (y % 2 == 0) {
    return y * MATRIX_WIDTH + x;
  } else {
    return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
  }
}

void setup() {
  WiFi.softAP(ssid, password);
  strip.begin();
  strip.show();
  server.on("/", handleRoot);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/rotate", handleRotate);
  server.on("/down", handleDown);
  server.on("/restart", handleRestart);
  server.begin();
  spawnTetromino();
}

// Spawn a new tetromino at the top
void spawnTetromino() {
  currentTetromino = random(0, 7);
  rotation = 0;
  posX = 3; // Centered
  posY = 0;
}

// Check collision for current piece at (x, y) with rotation
bool checkCollision(int x, int y, int rot) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (tetrominos[currentTetromino][i][j]) {
        int nx = x + j;
        int ny = y + i;
        if (nx < 0 || nx >= MATRIX_WIDTH || ny < 0 || ny >= MATRIX_HEIGHT) return true;
        if (board[ny][nx]) return true;
      }
    }
  }
  return false;
}

// Place current piece on the board
void placeTetromino() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (tetrominos[currentTetromino][i][j]) {
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
      if (tetrominos[currentTetromino][i][j]) {
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
      rotated[j][3-i] = tetrominos[currentTetromino][i][j];
    }
  }
  // Check collision for rotated piece
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (rotated[i][j]) {
        int nx = posX + j;
        int ny = posY + i;
        if (nx < 0 || nx >= MATRIX_WIDTH || ny < 0 || ny >= MATRIX_HEIGHT || board[ny][nx]) {
          return; // Collision, do not rotate
        }
      }
    }
  }
  // Apply rotation
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      tetrominos[currentTetromino][i][j] = rotated[i][j];
    }
  }
  drawBoard();
}

// Enhanced root handler with score and controls
void handleRoot() {
  String html = "<html><head><title>Tetris ESP8266</title></head><body>";
  html += "<h1>Tetris for ESP8266</h1>";
  html += "<p>Score: " + String(score) + "</p>";
  if (gameOver) {
    html += "<h2>Game Over!</h2>";
    html += "<button onclick=\"fetch('/restart')\">Restart</button> ";
  } else {
    html += "<button onclick=\"fetch('/left')\">Left</button> ";
    html += "<button onclick=\"fetch('/right')\">Right</button> ";
    html += "<button onclick=\"fetch('/rotate')\">Rotate</button> ";
    html += "<button onclick=\"fetch('/down')\">Down</button> ";
  }
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleRestart() {
  memset(board, 0, sizeof(board));
  score = 0;
  gameOver = false;
  spawnTetromino();
  drawBoard();
  server.send(200, "text/plain", "OK");
}

// Handle left movement
void handleLeft() {
  if (!checkCollision(posX - 1, posY, rotation)) {
    posX--;
    drawBoard();
  }
  server.send(200, "text/plain", "OK");
}

// Handle right movement
void handleRight() {
  if (!checkCollision(posX + 1, posY, rotation)) {
    posX++;
    drawBoard();
  }
  server.send(200, "text/plain", "OK");
}

// Handle rotation
void handleRotate() {
  rotateTetromino();
  server.send(200, "text/plain", "OK");
}

// Handle down movement
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
  drawBoard();
  server.send(200, "text/plain", "OK");
}

unsigned long lastDrop = 0;
const unsigned long dropInterval = 600; // ms
bool gameOver = false;

void loop() {
  server.handleClient();
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
    drawBoard();
  }
  delay(10);
}
