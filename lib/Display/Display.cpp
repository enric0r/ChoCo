#include "Display.h"
#include "ChordEngine.h"
#include "Config.h"
#include "DisplayDirection.h"
#include <Wire.h>
#include <stdio.h>
#include <string.h>

namespace {

// Both screensaver lines fit inside this moving block, including C# HMIN.
static constexpr int16_t kSaverWidth = 72;
static constexpr int16_t kSaverHeight = 36;

static char g_statusBody[STATUS_TEXT_CAPACITY] = {};
static bool displayReady = false;
static char g_statusLabel[8] = "INFO";
static uint32_t g_statusStart = 0;
static uint32_t g_statusDuration = 0;
static bool g_editModeIndicator = false;

struct InteractionState {
  char rawKey = '\0';
  JoystickDirection joyDirection = JoystickDirection::Center;
  bool modifierCHeld = false;
  bool joyBtnHeld = false;
};

struct UiSnapshot {
  const char* chordName;
  const char* rootName;
  const char* scaleAbbr;
  bool autoVoicing;
  bool bassMode;
  bool singleNoteMode;
  bool strumMode;
  bool latchMode;
  bool editMode;
  bool hasStatusOverlay;
  int activeDegree;
  int suggestionCount;
  int suggestionDegrees[4];
  JoystickDirection joyDirection;
  bool joyDirectionActive;
};

static InteractionState g_interactionState;
static unsigned long lastActivityTime = 0;
static bool screensaverActive = false;

void presentFrame() {
  static uint8_t previous[SCREEN_WIDTH * SCREEN_HEIGHT / 8] = {};
  static bool valid = false;
  const uint8_t* frame = display.getBuffer();
  if (!valid || memcmp(previous, frame, sizeof(previous)) != 0) {
    display.display();
    memcpy(previous, frame, sizeof(previous));
    valid = true;
  }
}

void scanI2C(TwoWire& tw) {
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
  Serial.println(F("\nScanning I2C bus for devices..."));
  for (uint8_t addr = 1; addr < 127; addr++) {
    tw.beginTransmission(addr);
    uint8_t error = tw.endTransmission();
    if (error == 0) {
      Serial.print(F("I2C device found at address 0x"));
      if (addr < 16) {
        Serial.print("0");
      }
      Serial.println(addr, HEX);
    }
  }
  Serial.println(F("Scan complete.\n"));
#else
  (void)tw;
#endif
}

void copyText(char* dest, size_t size, const char* src) {
  if (size == 0) {
    return;
  }
  strncpy(dest, src, size - 1);
  dest[size - 1] = '\0';
}

void drawCenteredText(const char* text, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t size, uint16_t color, uint16_t bg = SSD1306_BLACK) {
  display.setTextSize(size);
  display.setTextColor(color, bg);
  int16_t x1;
  int16_t y1;
  uint16_t textW;
  uint16_t textH;
  display.getTextBounds(text, 0, 0, &x1, &y1, &textW, &textH);
  int16_t cursorX = x + ((w - (int16_t)textW) / 2);
  int16_t cursorY = y + ((h - (int16_t)textH) / 2) - y1;
  if (cursorX < x + 1) {
    cursorX = x + 1;
  }
  display.setCursor(cursorX, cursorY);
  display.print(text);
}

void drawDirectionGlyph(int16_t x, int16_t y, JoystickDirection direction) {
  // Preserve the mounted device's vertical orientation; horizontal is not mirrored.
  const DirectionVector vector = displayDirectionVector(direction);
  const int16_t dx = vector.x * 4, dy = vector.y * 4;
  if (dx == 0 && dy == 0) return;
  const int16_t cx = x + 5, cy = y + 5;
  display.drawLine(cx - dx, cy - dy, cx + dx, cy + dy, SSD1306_WHITE);
  const int16_t tipX = cx + dx, tipY = cy + dy;
  display.drawLine(tipX, tipY, tipX - vector.x * 3 + vector.y * 2,
                   tipY - vector.y * 3 - vector.x * 2, SSD1306_WHITE);
  display.drawLine(tipX, tipY, tipX - vector.x * 3 - vector.y * 2,
                   tipY - vector.y * 3 + vector.x * 2, SSD1306_WHITE);
}

// Fixed-width built-in font: fit text into the reserved line with explicit truncation.
void drawTextLine(const char* text, int16_t x, int16_t y, int16_t width) {
  char fitted[CHORD_NAME_CAPACITY];
  const size_t capacity = min(sizeof(fitted) - 1, size_t(width / 6));
  copyText(fitted, capacity + 1, text);
  if (strlen(text) > capacity && capacity >= 3) {
    memcpy(fitted + capacity - 3, "...", 3);
  }
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);
  display.print(fitted);
}

bool statusVisible() {
  return g_statusBody[0] && uint32_t(millis() - g_statusStart) < g_statusDuration;
}

void parseStatusMessage(const char* message) {
  struct Prefix { const char* text; const char* label; };
  static const Prefix prefixes[] = {
    {"Root: ", "ROOT"}, {"Scale: ", "SCALE"}, {"Latch: ", "LATCH"},
    {"Bass: ", "BASS"}, {"Strum: ", "STRM"}, {"SmartVoice: ", "VOICE"},
    {"Note: ", "NOTE"}, {"Octave: ", "OCT"}, {"Joy: ", "JOY"}
  };
  copyText(g_statusLabel, sizeof(g_statusLabel), "INFO");
  for (const auto& prefix : prefixes) {
    const size_t length = strlen(prefix.text);
    if (strncmp(message, prefix.text, length) == 0) {
      copyText(g_statusLabel, sizeof(g_statusLabel), prefix.label);
      message += length;
      break;
    }
  }
  if (strncmp(message, "Deg ", 4) == 0) copyText(g_statusLabel, sizeof(g_statusLabel), "INV");
  copyText(g_statusBody, sizeof(g_statusBody), message);
}

UiSnapshot buildSnapshot() {
  UiSnapshot snapshot = {};
  snapshot.chordName = getCurrentChordName();
  snapshot.rootName = getNoteName(getCurrentRootNote());
  snapshot.scaleAbbr = getScaleAbbreviation(getCurrentScaleName());
  snapshot.autoVoicing = isAutoVoicingMode();
  snapshot.bassMode = isBassMode();
  snapshot.singleNoteMode = isSingleNoteMode();
  snapshot.strumMode = isStrumMode();
  snapshot.latchMode = isChordLatchMode();
  snapshot.editMode = g_editModeIndicator;
  snapshot.hasStatusOverlay = statusVisible();
  snapshot.activeDegree = getActiveChordDegree();
  snapshot.suggestionCount = min(4, getChordSuggestionCount());
  snapshot.joyDirection = g_interactionState.joyDirection;
  snapshot.joyDirectionActive = (snapshot.joyDirection != JoystickDirection::Center);

  for (int i = 0; i < snapshot.suggestionCount; ++i) {
    snapshot.suggestionDegrees[i] = getChordSuggestionDegree(i);
  }

  return snapshot;
}

void drawTopBar(const UiSnapshot& snapshot) {
  char keyLabel[16];
  snprintf(keyLabel, sizeof(keyLabel), "%s %s", snapshot.rootName, snapshot.scaleAbbr);
  drawTextLine(keyLabel, 3, 2, 72);
  if (snapshot.editMode) {
    drawTextLine("EDIT", 80, 2, 24);
  } else if (isValidDegree(snapshot.activeDegree)) {
    char degree[3] = {'D', static_cast<char>('1' + snapshot.activeDegree), '\0'};
    drawTextLine(degree, 92, 2, 12);
  }
  if (snapshot.joyDirectionActive) drawDirectionGlyph(114, 0, snapshot.joyDirection);
}

void drawHeroChord(const UiSnapshot& snapshot) {
  const char* label = snapshot.chordName[0] ? snapshot.chordName : "READY";
  const uint8_t size = strlen(label) <= 10 ? 2 : 1;
  if (strlen(label) > 20) {
    drawTextLine(label, 4, 23, 120);
  } else {
    drawCenteredText(label, 2, 16, 124, 22, size, SSD1306_WHITE);
  }
}

void drawActiveModes(const UiSnapshot& snapshot) {
  char modes[24] = {};
  // Short text labels are easier to recognize than six competing filled icons.
  if (snapshot.autoVoicing) strcat(modes, "VOI ");
  if (snapshot.bassMode) strcat(modes, "BAS ");
  if (snapshot.singleNoteMode) strcat(modes, "NOTE ");
  if (snapshot.strumMode) strcat(modes, "STR ");
  if (snapshot.latchMode) strcat(modes, "LAT ");
  const size_t length = strlen(modes);
  if (length) {
    modes[length - 1] = '\0';
    drawCenteredText(modes, 2, 41, 124, 8, 1, SSD1306_WHITE);
  }
}

void drawFooter(const UiSnapshot& snapshot) {
  display.drawFastHLine(3, 52, 122, SSD1306_WHITE);
  if (snapshot.hasStatusOverlay) {
    char message[STATUS_TEXT_CAPACITY + 8];
    if (strcmp(g_statusLabel, "INFO") == 0 || strcmp(g_statusLabel, "INV") == 0)
      snprintf(message, sizeof(message), "%s", g_statusBody);
    else
      snprintf(message, sizeof(message), "%s %s", g_statusLabel, g_statusBody);
    drawTextLine(message, 3, 56, 122);
    return;
  }
  drawTextLine("NEXT", 3, 56, 24);
  for (int i = 0; i < snapshot.suggestionCount; ++i) {
    const int degree = snapshot.suggestionDegrees[i];
    if (!isValidDegree(degree)) continue;
    char label[2] = {static_cast<char>('1' + degree), '\0'};
    drawTextLine(label, 43 + i * 22, 56, 6);
  }
}

void drawSplashFrame() {
  display.clearDisplay();
  drawCenteredText("CHOCO", 0, 16, SCREEN_WIDTH, 24, 3, SSD1306_WHITE);
  drawCenteredText("USB MIDI", 0, 46, SCREEN_WIDTH, 8, 1, SSD1306_WHITE);
}

void drawScreensaverFrame(int16_t x, int16_t y) {
  display.clearDisplay();
  drawCenteredText("CHOCO", x, y, kSaverWidth, 16, 2, SSD1306_WHITE);
  char context[16];
  snprintf(context, sizeof(context), "%s %s", getNoteName(getCurrentRootNote()),
           getScaleAbbreviation(getCurrentScaleName()));
  drawCenteredText(context, x, y + 24, kSaverWidth, 8, 1, SSD1306_WHITE);
}

} // namespace

#if I2C_PORT == 0
#define CHOCO_I2C_BUS Wire
#else
#define CHOCO_I2C_BUS Wire1
#endif

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &CHOCO_I2C_BUS, OLED_RESET);

void setupDisplay() {
  TwoWire& tw = CHOCO_I2C_BUS;

  tw.setSDA(I2C_SDA_PIN);
  tw.setSCL(I2C_SCL_PIN);
  tw.begin();
  tw.setClock(400000);

#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
  scanI2C(tw);

  Serial.print(F("Initializing display at address 0x"));
  Serial.println(SCREEN_ADDRESS, HEX);
#endif

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
    Serial.println(F("SSD1306 allocation failed"));
#endif
    return;
  }

  displayReady = true;
  display.clearDisplay();
  display.setTextWrap(false);
  resetScreensaverTimer();
}

void resetScreensaverTimer() {
  lastActivityTime = millis();
  if (screensaverActive) {
    screensaverActive = false;
    if (displayReady) display.clearDisplay();
  }
}

bool isScreensaverActive() {
  return screensaverActive;
}

void updateScreensaver() {
  if (!displayReady) return;
  static unsigned long lastAnimationUpdate = 0;
  static int16_t x = 0;
  static int16_t y = 16;
  static int16_t dx = 2;
  static int16_t dy = 1;

  if (!screensaverActive && (millis() - lastActivityTime > SCREENSAVER_TIMEOUT_MS)) {
    screensaverActive = true;
    lastAnimationUpdate = millis() - SCREENSAVER_ANIMATION_INTERVAL_MS;
  }

  if (!screensaverActive) {
    return;
  }

  if (millis() - lastAnimationUpdate < SCREENSAVER_ANIMATION_INTERVAL_MS) {
    return;
  }

  lastAnimationUpdate = millis();
  x += dx;
  y += dy;
  if (x <= 0 || x >= SCREEN_WIDTH - kSaverWidth) {
    dx = -dx;
    x += dx;
  }
  if (y <= 0 || y >= SCREEN_HEIGHT - kSaverHeight) {
    dy = -dy;
    y += dy;
  }

  drawScreensaverFrame(x, y);
  presentFrame();
}

void drawSplashScreen() {
  if (!displayReady) return;
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
  Serial.println("Drawing splash screen...");
#endif
  unsigned long start = millis();
  drawSplashFrame();
  presentFrame();
  while (millis() - start < SPLASH_SCREEN_DURATION) {
    delay(160);
  }

  display.clearDisplay();
  presentFrame();
  delay(150);
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
  Serial.println("Splash screen complete");
#endif
}

void setEditModeIndicator(bool enabled) {
  g_editModeIndicator = enabled;
}

void setInteractionState(char rawKey, JoystickDirection direction, bool modifierCHeld, bool joyBtnHeld) {
  g_interactionState.rawKey = rawKey;
  g_interactionState.joyDirection = direction;
  g_interactionState.modifierCHeld = modifierCHeld;
  g_interactionState.joyBtnHeld = joyBtnHeld;
}

void updateDisplay() {
  if (!displayReady || screensaverActive) {
    return;
  }

  const UiSnapshot snapshot = buildSnapshot();
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  drawTopBar(snapshot);
  drawHeroChord(snapshot);
  drawActiveModes(snapshot);
  drawFooter(snapshot);

  presentFrame();
}

void showStatus(const char* message, unsigned long durationMs) {
  parseStatusMessage(message ? message : "");
  g_statusStart = millis();
  g_statusDuration = durationMs;
}

void showStatusValue(const char* label, const char* value, unsigned long durationMs) {
  char message[STATUS_TEXT_CAPACITY];
  snprintf(message, sizeof(message), "%s: %s", label, value);
  showStatus(message, durationMs);
}

void showStatusNumber(const char* label, int value, unsigned long durationMs) {
  char number[12];
  snprintf(number, sizeof(number), "%d", value);
  showStatusValue(label, number, durationMs);
}

const char* getScaleAbbreviation(const char* fullName) {
  if (fullName == nullptr) {
    return "";
  }
  if (strcmp(fullName, "Ionian") == 0) {
    return "ION";
  }
  if (strcmp(fullName, "Dorian") == 0) {
    return "DOR";
  }
  if (strcmp(fullName, "Phrygian") == 0) {
    return "PHR";
  }
  if (strcmp(fullName, "Lydian") == 0) {
    return "LYD";
  }
  if (strcmp(fullName, "Mixolyd") == 0) {
    return "MIX";
  }
  if (strcmp(fullName, "Aeolian") == 0) {
    return "AEO";
  }
  if (strcmp(fullName, "Locrian") == 0) {
    return "LOC";
  }
  if (strcmp(fullName, "HarmMin") == 0) {
    return "HMIN";
  }
  if (strcmp(fullName, "MelMin") == 0) {
    return "MMIN";
  }
  return fullName;
}

void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h) {
  display.drawBitmap(x, y, bitmap, w, h, SSD1306_WHITE);
}
