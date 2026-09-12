// SameGame for ESP8266 with Adafruit NeoPixel 11x11 serpentine matrix
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiManager.h>
#include <WebSocketsServer.h>

#define LED_PIN D7
#define LED_COUNT 121
#define MATRIX_WIDTH 11
#define MATRIX_HEIGHT 11
#define COLOR_COUNT 4

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(80);
WiFiManager wifiManager;
WebSocketsServer webSocket = WebSocketsServer(81);

enum GameStatus {
  STATUS_PLAYING,
  STATUS_WON,
  STATUS_STUCK
};

uint8_t board[MATRIX_HEIGHT][MATRIX_WIDTH] = {0};
uint32_t gameColors[COLOR_COUNT];
GameStatus gameStatus = STATUS_PLAYING;
int score = 0;
int movesMade = 0;
int cursorX = MATRIX_WIDTH / 2;
int cursorY = MATRIX_HEIGHT / 2;
bool cursorBlinkVisible = true;
unsigned long lastBlinkToggle = 0;
const unsigned long blinkInterval = 350;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void handleRoot();
void handleRestart();
void handleClick();
void handleMove();
void handleFire();
void handleState();
void setupColors();
void startNewGame();
void fillRandomBoard();
void drawBoard();
void applyGravity();
void collapseColumns();
void broadcastState();
void sendStateToClient(uint8_t num);
String buildStateJson();
String statusToString();
bool boardIsEmpty();
bool hasPossibleMove();
bool removeGroupAt(int x, int y);
int collectGroup(int startX, int startY, int groupX[], int groupY[]);
bool inBounds(int x, int y);
void moveCursor(int dx, int dy);
void updateGameStatus();

// Map (x, y) to LED index for serpentine wiring starting at top-left
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
  Serial.println("\n\nStarting SameGame...");

  randomSeed(micros());

  wifiManager.autoConnect("SameGameClock");

  Serial.println("WiFi connected!");
  IPAddress ip = WiFi.localIP();
  Serial.print("Local IP address: ");
  Serial.println(ip);

  strip.begin();
  strip.show();
  setupColors();

  server.on("/", handleRoot);
  server.on("/restart", handleRestart);
  server.on("/click", handleClick);
  server.on("/move", handleMove);
  server.on("/fire", handleFire);
  server.on("/state", handleState);
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  startNewGame();
}

void setupColors() {
  gameColors[0] = strip.Color(100, 0, 5);   // red
  gameColors[1] = strip.Color(100, 90, 0);   // yellow
  gameColors[2] = strip.Color(10, 95, 0);   // green
  gameColors[3] = strip.Color(0, 20, 85);  // blue
}

String statusToString() {
  if (gameStatus == STATUS_WON) {
	return "won";
  }
  if (gameStatus == STATUS_STUCK) {
	return "stuck";
  }
  return "playing";
}

bool inBounds(int x, int y) {
  return x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT;
}

bool boardIsEmpty() {
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
	for (int x = 0; x < MATRIX_WIDTH; x++) {
	  if (board[y][x] != 0) {
		return false;
	  }
	}
  }
  return true;
}

bool hasPossibleMove() {
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
	for (int x = 0; x < MATRIX_WIDTH; x++) {
	  uint8_t color = board[y][x];
	  if (!color) {
		continue;
	  }
	  if (x + 1 < MATRIX_WIDTH && board[y][x + 1] == color) {
		return true;
	  }
	  if (y + 1 < MATRIX_HEIGHT && board[y + 1][x] == color) {
		return true;
	  }
	}
  }
  return false;
}

void fillRandomBoard() {
  int attempts = 0;
  do {
	for (int y = 0; y < MATRIX_HEIGHT; y++) {
	  for (int x = 0; x < MATRIX_WIDTH; x++) {
		board[y][x] = random(1, COLOR_COUNT + 1);
	  }
	}
	attempts++;
  } while (!hasPossibleMove() && attempts < 64);

  if (!hasPossibleMove()) {
	board[0][0] = 1;
	board[0][1] = 1;
  }
}

void startNewGame() {
  memset(board, 0, sizeof(board));
  score = 0;
  movesMade = 0;
  cursorX = MATRIX_WIDTH / 2;
  cursorY = MATRIX_HEIGHT / 2;
  cursorBlinkVisible = true;
  lastBlinkToggle = millis();
  gameStatus = STATUS_PLAYING;
  fillRandomBoard();
  drawBoard();
  broadcastState();
}

void drawBoard() {
  strip.clear();
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
	for (int x = 0; x < MATRIX_WIDTH; x++) {
	  uint8_t colorIndex = board[y][x];
	  if (colorIndex > 0) {
		strip.setPixelColor(xyToIndex(x, y), gameColors[colorIndex - 1]);
	  }
	}
  }
  if (cursorBlinkVisible && inBounds(cursorX, cursorY)) {
  strip.setPixelColor(xyToIndex(cursorX, cursorY), strip.Color(255, 255, 255));
  }
  strip.show();
}

int collectGroup(int startX, int startY, int groupX[], int groupY[]) {
  if (!inBounds(startX, startY) || board[startY][startX] == 0) {
	return 0;
  }

  bool visited[MATRIX_HEIGHT][MATRIX_WIDTH] = {false};
  int queueX[LED_COUNT];
  int queueY[LED_COUNT];
  int head = 0;
  int tail = 0;
  int groupSize = 0;
  uint8_t color = board[startY][startX];

  queueX[tail] = startX;
  queueY[tail] = startY;
  tail++;
  visited[startY][startX] = true;

  while (head < tail) {
	int x = queueX[head];
	int y = queueY[head];
	head++;

	groupX[groupSize] = x;
	groupY[groupSize] = y;
	groupSize++;

	const int dx[4] = {1, -1, 0, 0};
	const int dy[4] = {0, 0, 1, -1};
	for (int i = 0; i < 4; i++) {
	  int nx = x + dx[i];
	  int ny = y + dy[i];
	  if (inBounds(nx, ny) && !visited[ny][nx] && board[ny][nx] == color) {
		visited[ny][nx] = true;
		queueX[tail] = nx;
		queueY[tail] = ny;
		tail++;
	  }
	}
  }

  return groupSize;
}

void applyGravity() {
  for (int x = 0; x < MATRIX_WIDTH; x++) {
	int writeY = MATRIX_HEIGHT - 1;
	for (int y = MATRIX_HEIGHT - 1; y >= 0; y--) {
	  if (board[y][x] != 0) {
		board[writeY][x] = board[y][x];
		if (writeY != y) {
		  board[y][x] = 0;
		}
		writeY--;
	  }
	}
	while (writeY >= 0) {
	  board[writeY][x] = 0;
	  writeY--;
	}
  }
}

void collapseColumns() {
  int writeX = 0;
  for (int readX = 0; readX < MATRIX_WIDTH; readX++) {
	bool columnHasBlocks = false;
	for (int y = 0; y < MATRIX_HEIGHT; y++) {
	  if (board[y][readX] != 0) {
		columnHasBlocks = true;
		break;
	  }
	}

	if (!columnHasBlocks) {
	  continue;
	}

	if (writeX != readX) {
	  for (int y = 0; y < MATRIX_HEIGHT; y++) {
		board[y][writeX] = board[y][readX];
		board[y][readX] = 0;
	  }
	}
	writeX++;
  }

  for (int x = writeX; x < MATRIX_WIDTH; x++) {
	for (int y = 0; y < MATRIX_HEIGHT; y++) {
	  board[y][x] = 0;
	}
  }
}

bool removeGroupAt(int x, int y) {
  if (!inBounds(x, y) || board[y][x] == 0 || gameStatus != STATUS_PLAYING) {
	return false;
  }

  int groupX[LED_COUNT];
  int groupY[LED_COUNT];
  int groupSize = collectGroup(x, y, groupX, groupY);

  if (groupSize < 2) {
	return false;
  }

  for (int i = 0; i < groupSize; i++) {
	board[groupY[i]][groupX[i]] = 0;
  }

  score += groupSize * groupSize;
  movesMade++;

  applyGravity();
  collapseColumns();
  updateGameStatus();

  drawBoard();
  return true;
}

void updateGameStatus() {
  if (boardIsEmpty()) {
	gameStatus = STATUS_WON;
  } else if (!hasPossibleMove()) {
	gameStatus = STATUS_STUCK;
  } else {
	gameStatus = STATUS_PLAYING;
  }
}

void moveCursor(int dx, int dy) {
  int newX = cursorX + dx;
  int newY = cursorY + dy;

  if (newX < 0) {
	newX = 0;
  } else if (newX >= MATRIX_WIDTH) {
	newX = MATRIX_WIDTH - 1;
  }

  if (newY < 0) {
	newY = 0;
  } else if (newY >= MATRIX_HEIGHT) {
	newY = MATRIX_HEIGHT - 1;
  }

  if (newX == cursorX && newY == cursorY) {
	return;
  }

  cursorX = newX;
  cursorY = newY;
  cursorBlinkVisible = true;
  lastBlinkToggle = millis();
  drawBoard();
}

String buildStateJson() {
  String json = "{\"score\":" + String(score);
  json += ",\"moves\":" + String(movesMade);
  json += ",\"status\":\"" + statusToString() + "\"";
  json += ",\"cursorX\":" + String(cursorX);
  json += ",\"cursorY\":" + String(cursorY);
  json += ",\"board\":\"";
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
	for (int x = 0; x < MATRIX_WIDTH; x++) {
	  json += char('0' + board[y][x]);
	}
  }
  json += "\"}";
  return json;
}

void broadcastState() {
  String state = buildStateJson();
  webSocket.broadcastTXT(state);
}

void sendStateToClient(uint8_t num) {
  String state = buildStateJson();
  webSocket.sendTXT(num, state);
}

void handleState() {
  server.send(200, "application/json", buildStateJson());
}

void handleRestart() {
  startNewGame();
  server.send(200, "application/json", buildStateJson());
}

void handleClick() {
  handleFire();
}

void handleMove() {
  if (!server.hasArg("dir")) {
	server.send(400, "text/plain", "Missing dir");
	return;
  }

  String dir = server.arg("dir");
  if (dir == "up") {
	moveCursor(0, -1);
  } else if (dir == "down") {
	moveCursor(0, 1);
  } else if (dir == "left") {
	moveCursor(-1, 0);
  } else if (dir == "right") {
	moveCursor(1, 0);
  } else {
	server.send(400, "text/plain", "Invalid dir");
	return;
  }

  broadcastState();
  server.send(200, "application/json", buildStateJson());
}

void handleFire() {
  removeGroupAt(cursorX, cursorY);
  broadcastState();
  server.send(200, "application/json", buildStateJson());
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_CONNECTED) {
	sendStateToClient(num);
	return;
  }

  if (type != WStype_TEXT) {
	return;
  }

  String msg = "";
  for (size_t i = 0; i < length; i++) {
	msg += char(payload[i]);
  }

  if (msg == "restart") {
	startNewGame();
	return;
  }

  if (msg == "up") {
	moveCursor(0, -1);
	broadcastState();
	return;
  }

  if (msg == "down") {
	moveCursor(0, 1);
	broadcastState();
	return;
  }

  if (msg == "left") {
	moveCursor(-1, 0);
	broadcastState();
	return;
  }

  if (msg == "right") {
	moveCursor(1, 0);
	broadcastState();
	return;
  }

  if (msg == "fire") {
	removeGroupAt(cursorX, cursorY);
	broadcastState();
	return;
  }

	sendStateToClient(num);
}

void handleRoot() {
  String html = "<html><head><title>SameGame ESP8266</title>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;background:#111827;color:#f3f4f6;margin:0;padding:16px;text-align:center;}";
  html += ".wrap{max-width:420px;margin:0 auto;}";
  html += ".stats{display:flex;gap:12px;justify-content:center;flex-wrap:wrap;margin:14px 0;}";
  html += ".card{background:#1f2937;border-radius:12px;padding:10px 14px;min-width:110px;}";
  html += ".controls{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;max-width:260px;margin:18px auto 0;}";
  html += ".spacer{visibility:hidden;}";
  html += "button.action{margin-top:14px;border:0;border-radius:10px;padding:12px 18px;font-size:16px;background:#2563eb;color:white;cursor:pointer;}";
  html += "button.ctrl{border:0;border-radius:14px;padding:16px 0;font-size:22px;font-weight:bold;background:#374151;color:white;cursor:pointer;}";
  html += "button.fire{background:#dc2626;}";
  html += "p.note{color:#cbd5e1;line-height:1.45;}";
  html += "</style></head><body><div class='wrap'>";
  html += "<h1>SameGame</h1>";
  html += "<p class='note'>Das Spielfeld ist nur auf der Uhr sichtbar. Das aktive Feld startet in der Mitte und blinkt weiss. Bewege den Cursor mit den Tasten und l&ouml;se mit FIRE die aktuelle Position aus.</p>";
  html += "<div class='stats'>";
  html += "<div class='card'><div>Punkte</div><strong id='score'>0</strong></div>";
  html += "<div class='card'><div>Züge</div><strong id='moves'>0</strong></div>";
  html += "<div class='card'><div>Status</div><strong id='status'>L&auml;uft</strong></div>";
  html += "<div class='card'><div>Cursor</div><strong id='cursor'>6,6</strong></div>";
  html += "</div>";
  html += "<div class='controls'>";
  html += "<div class='spacer'>.</div><button class='ctrl' onclick='sendCommand(\"up\")'>&#8593;</button><div class='spacer'>.</div>";
  html += "<button class='ctrl' onclick='sendCommand(\"left\")'>&#8592;</button><button class='ctrl fire' onclick='sendCommand(\"fire\")'>FIRE</button><button class='ctrl' onclick='sendCommand(\"right\")'>&#8594;</button>";
  html += "<div class='spacer'>.</div><button class='ctrl' onclick='sendCommand(\"down\")'>&#8595;</button><div class='spacer'>.</div>";
  html += "</div>";
  html += "<button class='action' onclick='restartGameClient()'>Neu starten</button>";
  html += "</div><script>";
  html += "var currentState=" + buildStateJson() + ";";
  html += "var ws=null;";
  html += "function statusText(s){if(s==='won')return 'Gewonnen!';if(s==='stuck')return 'Keine Z&uuml;ge mehr';return 'L&auml;uft';}";
  html += "function renderState(state){currentState=state;document.getElementById('score').innerText=state.score;document.getElementById('moves').innerText=state.moves;document.getElementById('status').innerHTML=statusText(state.status);document.getElementById('cursor').innerText=(state.cursorX+1)+','+(state.cursorY+1);}";
  html += "function sendCommand(cmd){if(ws&&ws.readyState===1){ws.send(cmd);return;}if(cmd==='fire'){fetch('/fire').then(function(r){return r.json();}).then(renderState);return;}fetch('/move?dir='+encodeURIComponent(cmd)).then(function(r){return r.json();}).then(renderState);}";
  html += "function restartGameClient(){if(ws&&ws.readyState===1){ws.send('restart');return;}fetch('/restart').then(function(r){return r.json();}).then(renderState);}";
  html += "document.addEventListener('keydown',function(e){if(e.repeat)return;switch(e.key){case 'ArrowUp':sendCommand('up');e.preventDefault();break;case 'ArrowDown':sendCommand('down');e.preventDefault();break;case 'ArrowLeft':sendCommand('left');e.preventDefault();break;case 'ArrowRight':sendCommand('right');e.preventDefault();break;case ' ':case 'Enter':sendCommand('fire');e.preventDefault();break;}});";
  html += "try{ws=new WebSocket('ws://'+location.hostname+':81/');ws.onmessage=function(e){try{renderState(JSON.parse(e.data));}catch(err){console.log(err);}};ws.onclose=function(){console.log('ws closed');};ws.onerror=function(err){console.log(err);};}catch(e){console.log('ws init failed');}";
  html += "renderState(currentState);";
  html += "</script></body></html>";

  server.send(200, "text/html", html);
}

void loop() {
  server.handleClient();
  webSocket.loop();

  if (millis() - lastBlinkToggle >= blinkInterval) {
	lastBlinkToggle = millis();
	cursorBlinkVisible = !cursorBlinkVisible;
	drawBoard();
  }
}

