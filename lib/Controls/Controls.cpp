#include "Controls.h"
#include "Config.h"
#include "Display.h"
#include "ChordEngine.h"
#include "JoystickDirection.h"
#include "DebouncedInput.h"

// Matrix is wired column-to-row (columns are inputs, rows are outputs)
// Keys mapping restored to the original working layout
// Rows: GP2 (row 0), GP1 (row 1), GP0 (row 2)

//This relfects the following mapped layout:
// A B C _
// 1 3 5 _
// 0 2 4 6
char keys[ROWS][COLS] = {
  {'0', '2', '4', '6'},   // Row 0 (GP2)
  {'1', '3', '5', NO_KEY},   // Row 1 (GP1)
  {'A', 'B', 'C', NO_KEY}       // Row 2 (GP0)
};

const int rowPins[ROWS] = KEYPAD_ROW_PINS;
const int colPins[COLS] = KEYPAD_COL_PINS;
static DebouncedInput keyInputs[10];
static DebouncedInput joyButton;
static uint16_t stableKeys = 0;
static JoystickDirection sensedDirection = JoystickDirection::Center;

static int keyIndex(char key) {
  if (key >= '0' && key <= '6') return key - '0';
  if (key >= 'A' && key <= 'C') return 7 + key - 'A';
  return -1;
}

static unsigned long lastJoystickMoveTime = 0;
// Track last joystick direction at file scope so we can prime it externally
static JoystickDirection g_lastDirection = JoystickDirection::Center;
static bool g_lastBtnHeld = false;
static unsigned long g_lastIgnoreLog = 0;

enum class JoystickChordMode : uint8_t {
  Default = 0,
  Extended,
  Chromatic
};

static JoystickChordMode g_joystickChordMode = JoystickChordMode::Default;

static const char* joystickModeName(JoystickChordMode mode) {
  switch (mode) {
    case JoystickChordMode::Default: return "DEFAULT";
    case JoystickChordMode::Extended: return "EXTEND";
    case JoystickChordMode::Chromatic: return "CHROM";
    default: return "DEFAULT";
  }
}

static void cycleJoystickChordMode() {
  if (g_joystickChordMode == JoystickChordMode::Default) {
    g_joystickChordMode = JoystickChordMode::Extended;
  } else if (g_joystickChordMode == JoystickChordMode::Extended) {
    g_joystickChordMode = JoystickChordMode::Chromatic;
  } else {
    g_joystickChordMode = JoystickChordMode::Default;
  }
}
static constexpr JoystickClassifierConfig kJoystickConfig = {
  JOYSTICK_CENTER_X,
  JOYSTICK_CENTER_Y,
  JOYSTICK_TRIGGER_ENGAGE_PCT,
  JOYSTICK_TRIGGER_RELEASE_PCT,
  0,
  1023
};
struct MatrixScanResult {
  char firstKey;
  bool modifierCHeld;
  uint16_t heldKeys;
};

// Helper to determine if the joystick button is currently held considering polarity
static inline bool isJoyButtonHeld() {
  int raw = digitalRead(JOYSTICK_BTN);
#if JOYSTICK_BUTTON_ACTIVE_LOW
  return raw == LOW;
#else
  return raw == HIGH;
#endif
}

// Custom keypad scanner - SWAPPED: scan rows, read columns.
// Returns both first detected key (for event flow) and C-modifier state.
static MatrixScanResult scanMatrixState() {
  MatrixScanResult result = {NO_KEY, false, 0};

  // Ensure all rows are inactive before scanning
  for (int rr = 0; rr < ROWS; rr++) {
    pinMode(rowPins[rr], INPUT);
  }
  // Scan through each row (set LOW to activate)
  for (int r = 0; r < ROWS; r++) {
    digitalWrite(rowPins[r], LOW);
    pinMode(rowPins[r], OUTPUT);
    delayMicroseconds(MATRIX_SETTLE_US);
    
    // Check each column (read for LOW = pressed)
    for (int c = 0; c < COLS; c++) {
      if (digitalRead(colPins[c]) == LOW) {
        const char foundKey = keys[r][c];
        const int index = keyIndex(foundKey);
        if (index >= 0) result.heldKeys |= uint16_t(1u << index);
        if (foundKey == 'C') {
          result.modifierCHeld = true;
        }
        if (result.firstKey == NO_KEY && foundKey != NO_KEY) {
          result.firstKey = foundKey;
        }
      }
    }
    pinMode(rowPins[r], INPUT);
  }
  return result;
}

static char debounceKeys(uint16_t mask) {
  char pressed = NO_KEY;
  const uint32_t now = millis();
  stableKeys = 0;
  for (int i = 0; i < 10; ++i) {
    const bool changed = keyInputs[i].update((mask & (1u << i)) != 0, now, INPUT_DEBOUNCE_MS);
    if (keyInputs[i].held()) {
      stableKeys |= uint16_t(1u << i);
      // One musical action per scan; simultaneous presses use matrix-independent
      // priority (lowest degree first), while all releases remain independent.
      if (changed && pressed == NO_KEY && i != 9)
        pressed = i < 7 ? char('0' + i) : char('A' + i - 7);
    }
  }
  return pressed;
}

char getKey() {
  return debounceKeys(scanMatrixState().heldKeys);
}

// Immediate, no-debounce read of the matrix
char getRawKey() {
  return scanMatrixState().firstKey;
}

// Debug-only helper: scan the entire matrix and list all pressed keys.
// Writes a space-separated string of pressed keys into `out`.
// Intended for logging on state transitions, not for high-frequency polling.
void debugScanPressedKeys(char* out, int maxLen) {
  int pos = 0;
  // Ensure all rows inactive
  for (int rr = 0; rr < ROWS; rr++) {
    pinMode(rowPins[rr], INPUT);
  }
  for (int r = 0; r < ROWS; r++) {
    digitalWrite(rowPins[r], LOW);
    pinMode(rowPins[r], OUTPUT);
    delayMicroseconds(MATRIX_SETTLE_US);
    for (int c = 0; c < COLS; c++) {
      if (digitalRead(colPins[c]) == LOW) {
        char k = keys[r][c];
        if (k != NO_KEY && pos < maxLen - 2) { // leave space for space + terminator
          out[pos++] = k;
          out[pos++] = ' ';
        }
      }
    }
    pinMode(rowPins[r], INPUT);
  }
  if (pos > 0 && pos < maxLen) {
    // Replace trailing space with terminator
    out[pos - 1] = '\0';
  } else if (pos == 0 && maxLen > 0) {
    out[0] = '\0';
  }
}

// Robust modifier detection: explicitly scan for 'C' being pressed
bool isModifierCHeld() {
  return scanMatrixState().modifierCHeld;
}

void pollControls(ControlSnapshot& out) {
  const MatrixScanResult state = scanMatrixState();
  const int rawX = analogRead(JOYSTICK_X);
  const int rawY = analogRead(JOYSTICK_Y);

  int logicalX = JOYSTICK_SWAP_AXES ? rawY : rawX;
  int logicalY = JOYSTICK_SWAP_AXES ? rawX : rawY;
#if JOYSTICK_INVERT_X
  logicalX = 1023 - logicalX;
#endif
#if JOYSTICK_INVERT_Y
  logicalY = 1023 - logicalY;
#endif

  out.rawKey = state.firstKey;
  out.debouncedKey = debounceKeys(state.heldKeys);
  out.heldKeys = stableKeys;
  out.modifierCHeld = (stableKeys & (1u << 9)) != 0;
  out.joyX = logicalX;
  out.joyY = logicalY;
  joyButton.update(isJoyButtonHeld(), millis(), INPUT_DEBOUNCE_MS);
  out.joyBtnHeld = joyButton.held();
  sensedDirection = classifyJoystickDirectionLatchedWithConfig(out.joyX, out.joyY, sensedDirection, kJoystickConfig);
  out.joyDirection = sensedDirection;
}

void setupControls() {
  pinMode(JOYSTICK_BTN, INPUT_PULLUP);
  analogReadResolution(10);
  // Inactive rows float: two keys in a column must never short HIGH to LOW.
  for (int r = 0; r < ROWS; ++r) pinMode(rowPins[r], INPUT);
  for (int c = 0; c < COLS; ++c) pinMode(colPins[c], INPUT_PULLUP);
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_INFO
  Serial.println("Controls setup complete");
#endif
}

void handleKeyPress(char key) {
  // Normal chord playing
  if (key >= '0' && key <= '6') {
    playChordForDegree(key - '0');
  }
  else if (key == 'A') {
    int newRoot = (getCurrentRootNote() + 1);
    setCurrentRootNote(newRoot);
    showStatusValue("Root", getNoteName(getCurrentRootNote()), 300);
  }
  else if (key == 'B') {
    cycleScaleType(1);
    showStatusValue("Scale", getCurrentScaleName(), 600);
  }
  // Button C itself doesn't do anything when pressed alone - only used as modifier
}

JoystickDirection classifyJoystickDirectionInstant(int x, int y) {
  return classifyJoystickDirectionInstantWithConfig(x, y, kJoystickConfig);
}

static bool isMajorLikeQuality(TriadQuality quality) {
  return quality == TRIAD_QUALITY_MAJOR || quality == TRIAD_QUALITY_AUGMENTED;
}

bool getChordVariationForDirection(JoystickDirection direction, int degree, const int*& intervals, int& size, const char*& name) {
  intervals = nullptr;
  size = 0;
  name = nullptr;

  if (direction == JoystickDirection::Center) {
    return false;
  }

  const TriadQuality baseQuality = getTriadQualityForDegree(degree);
  const bool majorLike = isMajorLikeQuality(baseQuality);

  if (g_joystickChordMode == JoystickChordMode::Default) {
    switch (direction) {
      case JoystickDirection::Up:
        intervals = majorLike ? CHORD_MIN : CHORD_MAJ;
        size = 3;
        name = majorLike ? "Min" : "Maj";
        return true;
      case JoystickDirection::Down:
        intervals = CHORD_SUS4; size = 3; name = "Sus4"; return true;
      case JoystickDirection::Left:
        intervals = majorLike ? CHORD_DIM : CHORD_MIN;
        size = 3;
        name = majorLike ? "Dim" : "Min";
        return true;
      case JoystickDirection::Right:
        intervals = majorLike ? CHORD_MAJ7 : CHORD_MIN7;
        size = 4;
        name = majorLike ? "Maj7" : "Min7";
        return true;
      case JoystickDirection::UpLeft:
        intervals = CHORD_AUG; size = 3; name = "Aug"; return true;
      case JoystickDirection::UpRight:
        intervals = CHORD_DOM7; size = 4; name = "Dom7"; return true;
      case JoystickDirection::DownLeft:
        intervals = majorLike ? CHORD_MAJ6 : CHORD_SUS2;
        size = majorLike ? 4 : 3;
        name = majorLike ? "Maj6" : "Sus2";
        return true;
      case JoystickDirection::DownRight:
        intervals = majorLike ? CHORD_MAJ9 : CHORD_MIN9;
        size = 5;
        name = majorLike ? "Maj9" : "Min9";
        return true;
      default:
        return false;
    }
  }

  if (g_joystickChordMode == JoystickChordMode::Extended) {
    switch (direction) {
      case JoystickDirection::Up:
        intervals = majorLike ? CHORD_MIN : CHORD_MAJ;
        size = 3;
        name = majorLike ? "Min" : "Maj";
        return true;
      case JoystickDirection::Down:
        intervals = CHORD_DOM7_SHARP9; size = 5; name = "7#9"; return true;
      case JoystickDirection::Left:
        intervals = CHORD_SUS4_7; size = 4; name = "Sus4+7"; return true;
      case JoystickDirection::Right:
        intervals = CHORD_ADD11; size = 4; name = "Add11"; return true;
      case JoystickDirection::UpLeft:
        intervals = CHORD_HALFDIM7; size = 4; name = "m7b5"; return true;
      case JoystickDirection::UpRight:
        intervals = CHORD_DOM9; size = 5; name = "Dom9"; return true;
      case JoystickDirection::DownLeft:
        intervals = CHORD_ADD9; size = 4; name = "Add9"; return true;
      case JoystickDirection::DownRight:
        intervals = CHORD_MIN11; size = 5; name = "Min11"; return true;
      default:
        return false;
    }
  }

  // Chromatic mode
  switch (direction) {
    case JoystickDirection::Up:
      intervals = CHORD_MIN_MAJ7; size = 4; name = "mMaj7"; return true;
    case JoystickDirection::Down:
      intervals = CHORD_MAJ13; size = 5; name = "Maj13"; return true;
    case JoystickDirection::Left:
      intervals = CHORD_HALFDIM7; size = 4; name = "m7b5"; return true;
    case JoystickDirection::Right:
      intervals = CHORD_SIX_NINE; size = 5; name = "6/9"; return true;
    case JoystickDirection::UpLeft:
      intervals = CHORD_MAJ7_SHARP11; size = 5; name = "Maj7#11"; return true;
    case JoystickDirection::UpRight:
      intervals = CHORD_DOM13; size = 5; name = "Dom13"; return true;
    case JoystickDirection::DownLeft:
      intervals = CHORD_DOM7_FLAT9; size = 5; name = "7b9"; return true;
    case JoystickDirection::DownRight:
      intervals = CHORD_DOM7_ALT; size = 5; name = "7alt"; return true;
    default:
      return false;
  }
}

static bool isHorizontalDirection(JoystickDirection direction) {
  return direction == JoystickDirection::Left || direction == JoystickDirection::Right;
}

static void applyChordVariation(JoystickDirection direction) {
  if (direction == JoystickDirection::Center) return;

  // Do not play or change chords if none is currently held/active
  if (!isChordActive()) {
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_DEBUG
    Serial.println("Joystick variation ignored (no chord active)");
#endif
    return;
  }

  const int* intervals = nullptr;
  int size = 0;
  const char* name = nullptr;
  const int activeDegree = getActiveChordDegree();
  if (!getChordVariationForDirection(direction, activeDegree, intervals, size, name)) {
    return;
  }

#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_DEBUG
  Serial.print("Applying variation: "); Serial.println(name);
#endif

  if (isValidDegree(activeDegree)) {
    playChordForDegreeWithIntervals(activeDegree, intervals, size, name);
  } else {
    const int root = getActiveChordRoot();
    playChord(root, intervals, size, name);
  }
}

void handleJoystick(JoystickDirection direction, bool cHeld, bool held, char rawKey) {
  bool &lastBtnHeld = g_lastBtnHeld;
  unsigned long &lastIgnoreLog = g_lastIgnoreLog;
  static unsigned long btnPressStartTime = 0;
  static bool longPressHandled = false;
  static bool cHeldAtPress = false;
  static char keyAtPress = NO_KEY;

  if (held != lastBtnHeld) {
    if (held && !lastBtnHeld) {
      // Button just pressed
      btnPressStartTime = millis();
      longPressHandled = false;
      cHeldAtPress = cHeld; // Remember if C was held when button was pressed
      keyAtPress = rawKey;
    } else if (!held && lastBtnHeld) {
      // Button just released
      unsigned long pressDuration = millis() - btnPressStartTime;
      
      // Short press with C + A held toggles strum mode.
      // Short press with only C held toggles bass mode.
      if (!longPressHandled && pressDuration < JOYSTICK_SHORT_PRESS_MS && cHeldAtPress && keyAtPress == 'A') {
        toggleStrumMode();
        showStatusValue("Strum", (isStrumMode() ? "ON" : "OFF"), 600);
      } else if (!longPressHandled && pressDuration < JOYSTICK_SHORT_PRESS_MS && cHeldAtPress) {
        toggleBassMode();
        showStatusValue("Bass", (isBassMode() ? "ON" : "OFF"), 500);
      } else if (!longPressHandled && pressDuration < JOYSTICK_SHORT_PRESS_MS && !cHeldAtPress) {
        stopCurrentChord();
        showStatus("STOP", STATUS_MESSAGE_DURATION);
      }
    }
    lastBtnHeld = held;
  }
  
  // Check for long press while held - only if C was held at press time
  if (held && !longPressHandled && cHeldAtPress && (millis() - btnPressStartTime >= JOYSTICK_LONG_PRESS_MS)) {
    // Long press with C held - toggle smart voicing.
    toggleAutoVoicingMode();
    showStatusValue("SmartVoice", (isAutoVoicingMode() ? "ON" : "OFF"), 800);
    longPressHandled = true;
  }

  // Long press without C held toggles single note mode.
  if (held && !longPressHandled && !cHeldAtPress && (millis() - btnPressStartTime >= JOYSTICK_LONG_PRESS_MS)) {
    toggleSingleNoteMode();
    showStatusValue("Note", (isSingleNoteMode() ? "ON" : "OFF"), 800);
    longPressHandled = true;
  }


  // Track octave change state (left/right only).
  static JoystickDirection lastOctaveDirection = JoystickDirection::Center;
  static unsigned long lastOctaveChangeTime = 0;
  
  // Helper to update joystick tracking state
  auto updateJoystickState = [&]() {
    g_lastDirection = direction;
    lastJoystickMoveTime = millis();
  };
  
  // If C is held and joystick moved left/right, change octave.
  if (cHeld && isHorizontalDirection(direction)) {
    // Only change on direction change to avoid repeats
    if (direction != lastOctaveDirection && (millis() - lastOctaveChangeTime >= JOYSTICK_GRACE_PERIOD)) {
      if (direction == JoystickDirection::Right) {
        incrementOctave();
        showStatusNumber("Octave", getOctaveOffset(), 600);
      } else if (direction == JoystickDirection::Left) {
        decrementOctave();
        showStatusNumber("Octave", getOctaveOffset(), 600);
      }
      lastOctaveDirection = direction;
      lastOctaveChangeTime = millis();
      updateJoystickState();
      return; // Don't apply chord variation when changing octave
    }

    // If joystick is still in the same direction, just return without processing
    if (direction == lastOctaveDirection) {
      updateJoystickState();
      return;
    }
  } else {
    // Reset octave direction when the modifier is released or stick returns to center.
    if (!cHeld || direction == JoystickDirection::Center) {
      lastOctaveDirection = JoystickDirection::Center;
    }
  }

  // C + Down cycles joystick chord mode.
  static JoystickDirection lastModeDirection = JoystickDirection::Center;
  static unsigned long lastModeChangeTime = 0;
  if (cHeld && direction == JoystickDirection::Down) {
    if (direction != lastModeDirection && (millis() - lastModeChangeTime >= JOYSTICK_GRACE_PERIOD)) {
      cycleJoystickChordMode();
      showStatusValue("Joy", joystickModeName(g_joystickChordMode), 700);
      lastModeDirection = direction;
      lastModeChangeTime = millis();
      updateJoystickState();
      return;
    }
    if (direction == lastModeDirection) {
      updateJoystickState();
      return;
    }
  } else if (!cHeld || direction == JoystickDirection::Center) {
    lastModeDirection = JoystickDirection::Center;
  }

  if (cHeld) {
    updateJoystickState();
    return;
  }

  // Chromatic mode bonus: when no chord is held, left/right shifts key by semitone.
  static JoystickDirection lastChromaticDirection = JoystickDirection::Center;
  static unsigned long lastChromaticShiftTime = 0;
  if (!cHeld && g_joystickChordMode == JoystickChordMode::Chromatic && !isChordActive() && isHorizontalDirection(direction)) {
    if (direction != lastChromaticDirection && (millis() - lastChromaticShiftTime >= JOYSTICK_GRACE_PERIOD)) {
      int step = (direction == JoystickDirection::Right) ? 1 : -1;
      setCurrentRootNote(getCurrentRootNote() + step);
      showStatusValue("Root", getNoteName(getCurrentRootNote()), 500);
      lastChromaticDirection = direction;
      lastChromaticShiftTime = millis();
      updateJoystickState();
      return;
    }
    if (direction == lastChromaticDirection) {
      updateJoystickState();
      return;
    }
  } else if (direction == JoystickDirection::Center || cHeld || isChordActive()) {
    lastChromaticDirection = JoystickDirection::Center;
  }

  // If direction changed, apply/reset variation.
  bool changed = (direction != g_lastDirection);
  bool firstNonZero = (g_lastDirection == JoystickDirection::Center && direction != JoystickDirection::Center);
  if (!changed) {
    return;
  }

  // Return to base triad when joystick returns to center while a degree chord is active.
  if (direction == JoystickDirection::Center) {
    int activeDegree = getActiveChordDegree();
    if (isChordActive() && isValidDegree(activeDegree)) {
      playChordForDegree(activeDegree);
    }
    updateJoystickState();
    return;
  }

  if (firstNonZero || (millis() - lastJoystickMoveTime >= JOYSTICK_GRACE_PERIOD)) {
    if (!isChordActive()) {
      if (millis() - lastIgnoreLog >= JOYSTICK_LONG_PRESS_MS) {
#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_DEBUG
        Serial.println("Joystick variation ignored (no chord active)");
#endif
        lastIgnoreLog = millis();
      }
      updateJoystickState();
      return;
    }

#if CHOCO_LOG_LEVEL >= CHOCO_LOG_LEVEL_DEBUG
    Serial.print("Joystick direction="); Serial.println((int)direction);
#endif

    applyChordVariation(direction);
    updateJoystickState();
  }
}

void primeJoystickDirection(JoystickDirection direction) {
  g_lastDirection = direction;
  lastJoystickMoveTime = millis();
}

const char* getJoystickChordModeName() {
  return joystickModeName(g_joystickChordMode);
}
