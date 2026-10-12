/*
  Starbie - bichinho virtual para OLED 128x64 + MPU6050 + DHT11 (XIAO ESP32-C3)

  Ligacoes da SUA placa (conferidas no kicad2.kicad_pcb):
    OLED SDA / MPU6050 SDA -> D4 (GPIO6)
    OLED SCL / MPU6050 SCL -> D5 (GPIO7)
    DHT11 DATA             -> D1 (GPIO3)
    Botao 1                -> D2 (GPIO4)  (outro lado no GND, usa INPUT_PULLUP)
    Botao 2                -> D3 (GPIO5)  (outro lado no GND, usa INPUT_PULLUP)

  Bibliotecas: Adafruit GFX, Adafruit SSD1306, Adafruit MPU6050, DHT sensor library.
  Placa: XIAO_ESP32C3.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// ===================== CONFIGURACOES PARA INICIANTES =====================
const int I2C_SDA_PIN = 6;
const int I2C_SCL_PIN = 7;
const int DHT_PIN = 3;
const int BUTTON_ONE_PIN = 4;
const int BUTTON_TWO_PIN = 5;

// Se a sua versao nao tem DHT11, troque para false.
const bool USE_DHT11 = true;

// Se a inclinacao do menu ficar "trocada" ou "invertida" (depende de como o
// MPU6050 esta encaixado no J2), ajuste estas tres opcoes.
const bool TILT_SWAP_XY  = false;
const bool TILT_INVERT_X = false;
const bool TILT_INVERT_Y = false;

// Sensibilidade
const float TILT_DEADZONE   = 2.5;   // m/s^2 para escolher um setor
const float SHAKE_THRESHOLD = 16.0;  // quanto maior, mais forte tem que sacudir
// =========================================================================

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const uint8_t OLED_ADDRESS = 0x3C;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MPU6050 mpu;
DHT dht(DHT_PIN, DHT11);

// ---------------- Reacoes e menu radial ----------------
enum Reaction {
  NO_REACTION,
  NAP_REACTION,
  RUN_REACTION,
  JUMP_REACTION,
  HEART_REACTION,
  SHAKE_REACTION
};

struct MenuItem {
  const char* label;
  int8_t joy;
  int8_t energy;
  int8_t fullness;
  Reaction reaction;
};

// Ordem: 0 = cima, 1 = direita, 2 = baixo, 3 = esquerda
const MenuItem MENU_ITEMS[] = {
  {"NAP",    1,  18, -4, NAP_REACTION},    // top
  {"PLAY",  12,  -9, -5, RUN_REACTION},    // right
  {"FEED",   3,   2, 18, JUMP_REACTION},   // bottom
  {"DANCE", 15, -12, -3, JUMP_REACTION},   // left
};
const int MENU_COUNT = 4;

// ---------------- Sprite do bichinho (32x32, 1 bit) ----------------
const int PET_SPRITE_WIDTH = 32;
const int PET_SPRITE_HEIGHT = 32;

const uint8_t PROGMEM PET_SPRITE[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
  0x00, 0x03, 0x80, 0x00, 0x00, 0x03, 0x80, 0x00,
  0x00, 0x03, 0x80, 0x00, 0x00, 0x07, 0xC0, 0x00,
  0x00, 0x07, 0xC0, 0x00, 0x00, 0x0F, 0xE0, 0x00,
  0x00, 0x0F, 0xE0, 0x00, 0x00, 0x0F, 0xE0, 0x00,
  0x7F, 0xFF, 0xFF, 0xFC, 0x1F, 0xFF, 0xFF, 0xF0,
  0x0F, 0xFF, 0xFF, 0xE0, 0x07, 0xE7, 0xE7, 0xC0,
  0x03, 0xE7, 0xE7, 0x80, 0x01, 0xE7, 0xE7, 0x00,
  0x00, 0x7F, 0xFC, 0x00, 0x00, 0x7F, 0xFC, 0x00,
  0x00, 0x7F, 0xFC, 0x00, 0x00, 0x77, 0xEC, 0x00,
  0x00, 0xF8, 0x1E, 0x00, 0x00, 0xFF, 0xFE, 0x00,
  0x00, 0xFC, 0x7E, 0x00, 0x00, 0xF0, 0x1E, 0x00,
  0x01, 0xE0, 0x0F, 0x00, 0x01, 0x80, 0x03, 0x00,
  0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// ---------------- Estado ----------------
unsigned long now = 0;
unsigned long lastFrame = 0;
const unsigned long FRAME_MS = 33;

float joy = 60, energy = 60, fullness = 60;
unsigned long lastDecay = 0;

float petPos = 40;          // posicao horizontal "de passeio"
int petDir = 1;
unsigned long nextTurnAt = 0;
int petX = 0, petY = 0;     // posicao final de desenho
const int PET_BASE_Y = 22;
const int PET_MIN_X = 2;
const int PET_MAX_X = SCREEN_WIDTH - PET_SPRITE_WIDTH - 2;

Reaction currentReaction = NO_REACTION;
unsigned long reactionStart = 0;

bool menuOpen = false;
int menuHighlight = -1;
bool showStats = false;

bool mpuOk = false;
float accX = 0, accY = 0, accZ = 0;
float prevAccX = 0, prevAccY = 0, prevAccZ = 0;
float tiltX = 0, tiltY = 0;
unsigned long lastShakeAt = 0;
unsigned long lastSensorAt = 0;

float temperature = NAN, humidity = NAN;
unsigned long lastDhtAt = 0;

struct Button {
  int pin;
  bool stableState;      // true = solto
  bool lastReading;
  unsigned long changedAt;
  bool pressedEvent;
};
Button button1 = {BUTTON_ONE_PIN, true, true, 0, false};
Button button2 = {BUTTON_TWO_PIN, true, true, 0, false};

// ---------------- Utilidades ----------------
bool isNapping() {
  return currentReaction == NAP_REACTION;
}

unsigned long reactionDuration(Reaction r) {
  switch (r) {
    case NAP_REACTION:   return 8000;
    case RUN_REACTION:   return 2600;
    case JUMP_REACTION:  return 1200;
    case HEART_REACTION: return 2500;
    case SHAKE_REACTION: return 1200;
    default:             return 0;
  }
}

void startReaction(Reaction r) {
  currentReaction = r;
  reactionStart = now;
}

float reactionProgress() {
  unsigned long d = reactionDuration(currentReaction);
  if (d == 0) return 0;
  float t = (float)(now - reactionStart) / (float)d;
  if (t < 0) t = 0;
  if (t > 1) t = 1;
  return t;
}

float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ---------------- Botoes (com debounce) ----------------
void updateButton(Button &b) {
  b.pressedEvent = false;
  bool reading = digitalRead(b.pin) == HIGH;   // HIGH = solto (pull-up)
  if (reading != b.lastReading) {
    b.changedAt = now;
    b.lastReading = reading;
  }
  if ((now - b.changedAt) > 30 && reading != b.stableState) {
    b.stableState = reading;
    if (!reading) b.pressedEvent = true;       // acabou de apertar
  }
}

// ---------------- Sensores ----------------
void readSensors() {
  if (now - lastSensorAt < 30) return;
  lastSensorAt = now;

  if (mpuOk) {
    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);
    prevAccX = accX; prevAccY = accY; prevAccZ = accZ;
    accX = a.acceleration.x;
    accY = a.acceleration.y;
    accZ = a.acceleration.z;

    float tx = accX, ty = accY;
    if (TILT_SWAP_XY) { float tmp = tx; tx = ty; ty = tmp; }
    if (TILT_INVERT_X) tx = -tx;
    if (TILT_INVERT_Y) ty = -ty;
    tiltX = tx;
    tiltY = ty;
  } else {
    tiltX = 0;
    tiltY = 0;
  }

  if (USE_DHT11 && now - lastDhtAt > 3000) {
    lastDhtAt = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h)) humidity = h;
    if (!isnan(t)) temperature = t;
  }
}

bool detectShake() {
  if (!mpuOk) return false;
  float dx = accX - prevAccX;
  float dy = accY - prevAccY;
  float dz = accZ - prevAccZ;
  float jerk = sqrt(dx * dx + dy * dy + dz * dz);
  if (jerk > SHAKE_THRESHOLD && (now - lastShakeAt) > 1500) {
    lastShakeAt = now;
    return true;
  }
  return false;
}

// ---------------- Logica ----------------
void applyAction(int index) {
  const MenuItem &item = MENU_ITEMS[index];
  joy      = clampf(joy + item.joy, 0, 100);
  energy   = clampf(energy + item.energy, 0, 100);
  fullness = clampf(fullness + item.fullness, 0, 100);
  startReaction(item.reaction);
}

void updateMenuHighlight() {
  menuHighlight = -1;
  float ax = fabs(tiltX), ay = fabs(tiltY);
  if (ax < TILT_DEADZONE && ay < TILT_DEADZONE) return;
  if (ay >= ax) {
    menuHighlight = (tiltY < 0) ? 0 : 2;   // cima / baixo
  } else {
    menuHighlight = (tiltX > 0) ? 1 : 3;   // direita / esquerda
  }
}

void handleInput() {
  updateButton(button1);
  updateButton(button2);

  if (button1.pressedEvent) {
    if (!menuOpen) {
      menuOpen = true;
      showStats = false;
    } else {
      if (menuHighlight >= 0) applyAction(menuHighlight);
      menuOpen = false;
    }
  }

  if (button2.pressedEvent && !menuOpen) {
    showStats = !showStats;
  }

  if (menuOpen) {
    updateMenuHighlight();
  } else if (detectShake()) {
    startReaction(SHAKE_REACTION);   // tambem acorda do soninho
  }
}

void updateStats() {
  if (now - lastDecay < 5000) return;
  lastDecay = now;
  joy      = clampf(joy - 1, 0, 100);
  fullness = clampf(fullness - 1, 0, 100);
  if (isNapping()) energy = clampf(energy + 3, 0, 100);
  else             energy = clampf(energy - 0.5, 0, 100);
}

void updateReaction() {
  if (currentReaction == NO_REACTION) return;
  if (now - reactionStart >= reactionDuration(currentReaction)) {
    currentReaction = NO_REACTION;
  }
}

void updatePetWander(unsigned long dt) {
  if (currentReaction != NO_REACTION) return;   // parado durante reacoes
  petPos += petDir * 0.018f * dt;
  if (petPos <= PET_MIN_X) { petPos = PET_MIN_X; petDir = 1; }
  if (petPos >= PET_MAX_X) { petPos = PET_MAX_X; petDir = -1; }
  if (now > nextTurnAt) {
    if (random(0, 3) == 0) petDir = -petDir;
    nextTurnAt = now + random(1500, 4500);
  }
}

// ---------------- Desenho ----------------
void drawSparkle(int x, int y) {
  display.drawPixel(x, y - 2, SSD1306_WHITE);
  display.drawPixel(x - 1, y - 1, SSD1306_WHITE);
  display.drawPixel(x + 1, y - 1, SSD1306_WHITE);
  display.drawPixel(x - 2, y, SSD1306_WHITE);
  display.drawPixel(x, y, SSD1306_WHITE);
  display.drawPixel(x + 2, y, SSD1306_WHITE);
  display.drawPixel(x - 1, y + 1, SSD1306_WHITE);
  display.drawPixel(x + 1, y + 1, SSD1306_WHITE);
  display.drawPixel(x, y + 2, SSD1306_WHITE);
}

void drawHeart(int x, int y) {
  display.fillCircle(x - 2, y, 2, SSD1306_WHITE);
  display.fillCircle(x + 2, y, 2, SSD1306_WHITE);
  display.fillTriangle(x - 4, y + 1, x + 4, y + 1, x, y + 5, SSD1306_WHITE);
}

void drawFloatingHearts() {
  for (int i = 0; i < 3; i++) {
    int rise = (int)(((now / 40) + i * 9) % 26);
    int hx = petX + 6 + i * 10;
    int hy = petY - 2 - rise;
    if (hy > -6) drawHeart(hx, hy);
  }
}

void drawSleepZs() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < 3; i++) {
    int rise = (int)(((now / 150) + i * 5) % 16);
    display.setCursor(petX + 26 + i * 5, petY + 4 - rise);
    display.print(i == 1 ? "Z" : "z");
  }
}

void drawPet() {
  float t = reactionProgress();
  petX = (int)petPos;
  petY = PET_BASE_Y;

  switch (currentReaction) {
    case RUN_REACTION: {
      float p = t * 4.0f;                 // 4 trechos: ida, volta, ida, volta
      float v = fmod(p, 2.0f);
      float tri = (v < 1.0f) ? v : 2.0f - v;
      petX = PET_MIN_X + (int)((PET_MAX_X - PET_MIN_X) * tri);
      petPos = petX;
      break;
    }
    case JUMP_REACTION:
      petY -= (int)(fabs(sin(t * 6.2832f)) * 10.0f);   // dois pulinhos
      break;
    case SHAKE_REACTION:
      petX += ((now / 40) % 2 == 0) ? 3 : -3;
      break;
    default:
      break;
  }

  display.drawBitmap(petX, petY, PET_SPRITE, PET_SPRITE_WIDTH, PET_SPRITE_HEIGHT, SSD1306_WHITE);
  if (!isNapping() && (now / 250) % 2 == 0) {
    drawSparkle(petX - 4, petY + 18);
  }

  if (currentReaction == RUN_REACTION || currentReaction == HEART_REACTION) drawFloatingHearts();
  if (currentReaction == NAP_REACTION) drawSleepZs();
}

void drawMenuBox(int index, int x, int y) {
  const int w = 34, h = 11;
  const char* label = MENU_ITEMS[index].label;
  int textW = strlen(label) * 6;
  display.setTextSize(1);
  if (index == menuHighlight) {
    display.fillRect(x, y, w, h, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRect(x, y, w, h, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
  }
  display.setCursor(x + (w - textW) / 2, y + 2);
  display.print(label);
  display.setTextColor(SSD1306_WHITE);
}

void drawMenu() {
  drawMenuBox(0, 47, 0);     // cima
  drawMenuBox(1, 94, 26);    // direita
  drawMenuBox(2, 47, 53);    // baixo
  drawMenuBox(3, 0, 26);     // esquerda

  display.drawCircle(64, 32, 20, SSD1306_WHITE);
  int bx = 64 + (int)clampf(tiltX * 3.0f, -20, 20);
  int by = 32 + (int)clampf(tiltY * 3.0f, -18, 18);
  display.fillCircle(bx, by, 3, SSD1306_WHITE);
}

void drawBar(int y, const char* name, float value) {
  display.setCursor(0, y);
  display.print(name);
  display.drawRect(48, y, 78, 8, SSD1306_WHITE);
  int w = (int)(value * 0.74f);
  display.fillRect(50, y + 2, w, 4, SSD1306_WHITE);
}

void drawStats() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  drawBar(0,  "JOY",    joy);
  drawBar(12, "ENERGY", energy);
  drawBar(24, "FULL",   fullness);

  display.setCursor(0, 42);
  if (USE_DHT11 && !isnan(temperature)) {
    display.print("Temp: ");
    display.print(temperature, 1);
    display.print(" C");
    display.setCursor(0, 54);
    display.print("Umid: ");
    display.print(humidity, 0);
    display.print(" %");
  } else if (USE_DHT11) {
    display.print("DHT11: sem leitura");
  } else {
    display.print("DHT11 desligado");
  }
}

void render() {
  display.clearDisplay();
  if (showStats)      drawStats();
  else if (menuOpen)  drawMenu();
  else                drawPet();
  display.display();
}

// ---------------- Setup / loop ----------------
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_ONE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_TWO_PIN, INPUT_PULLUP);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED nao encontrado (confira SDA/SCL e o endereco 0x3C)");
    while (true) delay(1000);
  }
  display.clearDisplay();
  display.display();

  mpuOk = mpu.begin();
  if (mpuOk) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  } else {
    Serial.println("MPU6050 nao encontrado: menu por inclinacao e sacudir desativados");
  }

  if (USE_DHT11) dht.begin();

  randomSeed(micros());
  now = millis();
  lastFrame = now;
  lastDecay = now;
}

void loop() {
  now = millis();
  if (now - lastFrame < FRAME_MS) return;
  unsigned long dt = now - lastFrame;
  lastFrame = now;

  readSensors();
  handleInput();
  updateStats();
  updateReaction();
  updatePetWander(dt);
  render();
}
