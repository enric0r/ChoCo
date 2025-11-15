#include "Controls.h"
#include "Display.h"
#include "ChordEngine.h"

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

int rowPins[ROWS] = {2, 1, 0};      // OUTPUT (scan side) - SWAPPED
int colPins[COLS] = {3, 4, 5, 6};   // INPUT_PULLUP (read side) - SWAPPED

// Track last key state for edge detection
static char lastKey = NO_KEY;
static char lastReportedKey = NO_KEY;
static unsigned long lastDebounceTime = 0;
static const unsigned long debounceDelay = 10;

static unsigned long lastJoystickMoveTime = 0;
// Track last joystick direction at file scope so we can prime it externally
static int g_lastDx = 0;
static int g_lastDy = 0;
static bool g_lastBtnHeld = false;
static unsigned long g_lastIgnoreLog = 0;
static unsigned long g_lastBtnPressTime = 0;
static int g_btnPressCount = 0;

// Function button state (using button 'C' as function button)
static bool functionButtonHeld = false;
static char lastKeyRaw = NO_KEY;

// Helper to determine if the joystick button is currently held considering polarity
static inline bool isJoyButtonHeld() {
  int raw = digitalRead(JOYSTICK_BTN);
#if JOYSTICK_BUTTON_ACTIVE_LOW
  return raw == LOW;
#else
  return raw == HIGH;
#endif
}

// Custom keypad scanner - SWAPPED: scan rows, read columns
char scanKeypad() {
  // Ensure all rows are inactive before scanning
  for (int rr = 0; rr < ROWS; rr++) {
    digitalWrite(rowPins[rr], HIGH);
  }
  // Scan through each row (set LOW to activate)
  for (int r = 0; r < ROWS; r++) {
    digitalWrite(rowPins[r], LOW);
    delayMicroseconds(50);
    
    // Check each column (read for LOW = pressed)
    for (int c = 0; c < COLS; c++) {
      if (digitalRead(colPins[c]) == LOW) {
        char foundKey = keys[r][c];
        // Immediately deactivate this row before returning to avoid ghosting
        digitalWrite(rowPins[r], HIGH);
        // Serial.print("Raw key detected: ");
        // Serial.print(foundKey);
        // Serial.print(" at row=");
        // Serial.print(r);
        // Serial.print(" col=");
        // Serial.println(c);
        return foundKey;
      }
    }
    
    digitalWrite(rowPins[r], HIGH);
  }
  return NO_KEY;
}

char getKey() {
  char key = scanKeypad();
  
  // Debug logging
  static unsigned long lastDebugPrint = 0;
  static char lastDebugKey = NO_KEY;
  if (key != lastDebugKey || (millis() - lastDebugPrint > 5000 && key != NO_KEY)) {
    // Serial.print("getKey() scanKeypad returned: ");
    // Serial.print(key == NO_KEY ? "NO_KEY" : String(key));
    // Serial.print(", lastKey: ");
    // Serial.print(lastKey == NO_KEY ? "NO_KEY" : String(lastKey));
    // Serial.print(", lastReportedKey: ");
    // Serial.println(lastReportedKey == NO_KEY ? "NO_KEY" : String(lastReportedKey));
    lastDebugKey = key;
    lastDebugPrint = millis();
  }
  
  // Debounce: only update when key changes
  if (key != lastKey) {
    // Serial.print("Key changed from ");
    // Serial.print(lastKey == NO_KEY ? "NO_KEY" : String(lastKey));
    // Serial.print(" to ");
    // Serial.println(key == NO_KEY ? "NO_KEY" : String(key));
    lastDebounceTime = millis();
    lastKey = key;
    lastReportedKey = NO_KEY; // Reset so we can report the new key
  }
  
  // Report key if it's been stable for debounce period and we haven't reported it yet
  if ((millis() - lastDebounceTime) > debounceDelay && key != NO_KEY && key != lastReportedKey) {
    Serial.print("Key: ");
    Serial.println(key);
    lastReportedKey = key;
    return key;
  }
  
  // Clear reported key when released
  if (key == NO_KEY) {
    lastReportedKey = NO_KEY;
  }
  
  return NO_KEY;
}

// Immediate, no-debounce read of the matrix
char getRawKey() {
  return scanKeypad();
}

// Debug-only helper: scan the entire matrix and list all pressed keys.
// Writes a space-separated string of pressed keys into `out`.
// Intended for logging on state transitions, not for high-frequency polling.
void debugScanPressedKeys(char* out, int maxLen) {
  int pos = 0;
  // Ensure all rows inactive
  for (int rr = 0; rr < ROWS; rr++) {
    digitalWrite(rowPins[rr], HIGH);
  }
  for (int r = 0; r < ROWS; r++) {
    digitalWrite(rowPins[r], LOW);
    delayMicroseconds(50);
    for (int c = 0; c < COLS; c++) {
      if (digitalRead(colPins[c]) == LOW) {
        char k = keys[r][c];
        if (k != NO_KEY && pos < maxLen - 2) { // leave space for space + terminator
          out[pos++] = k;
          out[pos++] = ' ';
        }
      }
    }
    digitalWrite(rowPins[r], HIGH);
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
  bool held = false;
  // Ensure all rows inactive
  for (int rr = 0; rr < ROWS; rr++) {
    digitalWrite(rowPins[rr], HIGH);
  }
  for (int r = 0; r < ROWS; r++) {
    digitalWrite(rowPins[r], LOW);
    delayMicroseconds(50);
    for (int c = 0; c < COLS; c++) {
      if (digitalRead(colPins[c]) == LOW) {
        char k = keys[r][c];
        if (k == 'C') {
          held = true;
        }
      }
    }
    digitalWrite(rowPins[r], HIGH);
  }
  return held;
}

void setupControls() {
  Serial.println("Setting up controls...");
  Serial.println("SWAPPED WIRING: Rows=OUTPUT(scan), Cols=INPUT_PULLUP(read)");
  pinMode(JOYSTICK_BTN, INPUT_PULLUP);
  
  // Setup keypad matrix pins - SWAPPED from typical
  // Rows are OUTPUT (scan side)
  for (int r = 0; r < ROWS; r++) {
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], HIGH);
    Serial.print("Row pin ");
    Serial.print(rowPins[r]);
    Serial.println(" set to OUTPUT HIGH");
  }
  
  // Columns are INPUT_PULLUP (read side)
  for (int c = 0; c < COLS; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
    Serial.print("Col pin ");
    Serial.print(colPins[c]);
    Serial.print(" set to INPUT_PULLUP, current state: ");
    Serial.println(digitalRead(colPins[c]) ? "HIGH" : "LOW");
  }
  
  // Test scan immediately after setup
  Serial.println("Testing immediate scan...");
  char testKey = scanKeypad();
  if (testKey != NO_KEY) {
    Serial.print("Key detected during setup: ");
    Serial.println(testKey);
  } else {
    Serial.println("No key pressed during setup");
  }
  
  Serial.println("Controls setup complete");
}

void handleKeyPress(char key) {
  // Normal chord playing
  if (key >= '0' && key <= '7') {
    playChordForDegree(key - '0');
  }
  else if (key == 'A') {
    int newRoot = (getCurrentRootNote() + 1);
    setCurrentRootNote(newRoot);
    showStatus(String("Root: ") + getNoteName(getCurrentRootNote()), 300);
  }
  else if (key == 'B') {
    cycleScaleType(1);
    showStatus(String("Scale: ") + getCurrentScaleName(), 600);
  }
  // Button C itself doesn't do anything when pressed alone - only used as modifier
}

static int dirFromAxis(int v) {
  // Thresholds: <200 = -1, >800 = +1, otherwise 0
  // Make the joystick feel more responsive by shrinking the deadzone
  // Old: <200 / >800. New: <350 / >650 triggers with smaller deflection.
  if (v < 350) return -1;
  if (v > 650) return 1;
  return 0;
}

static void applyChordVariation(int dx, int dy) {
  if (dx == 0 && dy == 0) return;

  // Do not play or change chords if none is currently held/active
  if (!isChordActive()) {
    Serial.println("Joystick variation ignored (no chord active)");
    return;
  }

  const int root = getActiveChordRoot();
  const int *intervals = nullptr;
  int size = 0;
  String name = "";

  // Map 8-way directions to chord variations
  if (dx == 0 && dy == 1) {           // Up
    intervals = CHORD_MAJ7; size = 4; name = "Maj7";
  } else if (dx == 0 && dy == -1) {   // Down
    intervals = CHORD_MIN7; size = 4; name = "Min7";
  } else if (dx == -1 && dy == 0) {   // Left
    intervals = CHORD_SUS2; size = 3; name = "Sus2";
  } else if (dx == 1 && dy == 0) {    // Right
    intervals = CHORD_SUS4; size = 3; name = "Sus4";
  } else if (dx == 1 && dy == 1) {    // Up-Right
    intervals = CHORD_DOM9; size = 5; name = "9";
  } else if (dx == -1 && dy == 1) {   // Up-Left
    intervals = CHORD_DOM11; size = 5; name = "11";
  } else if (dx == 1 && dy == -1) {   // Down-Right
    intervals = CHORD_MIN9; size = 5; name = "Min9";
  } else if (dx == -1 && dy == -1) {  // Down-Left
    intervals = CHORD_DIM; size = 3; name = "Dim";
  }

  if (intervals != nullptr) {
    Serial.print("Applying variation: "); Serial.println(name);
    playChord(root, intervals, size, name);
  }
}

void handleJoystick(int x, int y) {
  int &lastDx = g_lastDx;
  int &lastDy = g_lastDy;
  bool &lastBtnHeld = g_lastBtnHeld;
  unsigned long &lastIgnoreLog = g_lastIgnoreLog;
  unsigned long &lastBtnPressTime = g_lastBtnPressTime;
  int &btnPressCount = g_btnPressCount;
  static unsigned long btnPressStartTime = 0;
  static bool longPressHandled = false;
  static bool cHeldAtPress = false;
  
  // Check if C modifier is held for function mode
  bool cHeld = isModifierCHeld();
  
  bool held = isJoyButtonHeld();
  if (held != lastBtnHeld) {
    if (held && !lastBtnHeld) {
      // Button just pressed
      btnPressStartTime = millis();
      longPressHandled = false;
      cHeldAtPress = cHeld; // Remember if C was held when button was pressed
    } else if (!held && lastBtnHeld) {
      // Button just released
      unsigned long pressDuration = millis() - btnPressStartTime;
      
      // Only toggle bass mode if C was held during the press
      if (!longPressHandled && pressDuration < 500 && cHeldAtPress) {
        // Short press with C held - toggle bass mode
        toggleBassMode();
        showStatus(String("Bass: ") + (isBassMode() ? "ON" : "OFF"), 500);
      }
    }
    lastBtnHeld = held;
  }
  
  // Check for long press while held - only if C was held at press time
  if (held && !longPressHandled && cHeldAtPress && (millis() - btnPressStartTime > 1000)) {
    // Long press with C held - toggle auto-voicing
    toggleAutoVoicingMode();
    showStatus(String("AutoVoice: ") + (isAutoVoicingMode() ? "ON" : "OFF"), 800);
    longPressHandled = true;
  }

  int dx = dirFromAxis(x);
  int dy = dirFromAxis(y);
  
  // Track octave change state
  static int lastOctaveDx = 0;
  static unsigned long lastOctaveChangeTime = 0;
  
  // Helper to update joystick tracking state
  auto updateJoystickState = [&]() {
    lastDx = dx;
    lastDy = dy;
    lastJoystickMoveTime = millis();
  };
  
  // If C is held and joystick moved left/right, change octave
  if (cHeld && dx != 0 && dy == 0) {
    // Only change on direction change to avoid repeats
    if (dx != lastOctaveDx && (millis() - lastOctaveChangeTime >= JOYSTICK_GRACE_PERIOD)) {
      if (dx > 0) {
        incrementOctave();
        showStatus(String("Octave: ") + getOctaveOffset(), 600);
      } else if (dx < 0) {
        decrementOctave();
        showStatus(String("Octave: ") + getOctaveOffset(), 600);
      }
      lastOctaveDx = dx;
      lastOctaveChangeTime = millis();
      updateJoystickState();
      return; // Don't apply chord variation when changing octave
    }
    // If joystick is still in the same direction, just return without processing
    if (dx == lastOctaveDx) {
      updateJoystickState();
      return;
    }
  } else {
    // Reset octave change tracking only when C is not held
    if (!cHeld) {
      lastOctaveDx = 0;
    }
  }

  // If direction changed, apply variation. Apply immediately on first move out of center;
  // otherwise respect the (reduced) grace period to avoid jitter.
  bool changed = (dx != lastDx || dy != lastDy);
  bool firstNonZero = (lastDx == 0 && lastDy == 0 && (dx != 0 || dy != 0));
  if (changed && (firstNonZero || (millis() - lastJoystickMoveTime >= JOYSTICK_GRACE_PERIOD))) {
    // Only require that a chord is currently active; do not require the joy button
    if (!isChordActive()) {
      if (millis() - lastIgnoreLog > 1000) {
        Serial.println("Joystick variation ignored (no chord active)");
        lastIgnoreLog = millis();
      }
      return;
    }

    Serial.print("Joystick raw x="); Serial.print(x);
    Serial.print(" y="); Serial.print(y);
    Serial.print(" -> dx="); Serial.print(dx);
    Serial.print(" dy="); Serial.println(dy);

    applyChordVariation(dx, dy);
    lastDx = dx;
    lastDy = dy;
    lastJoystickMoveTime = millis();
  }
}

void primeJoystickDirection(int dx, int dy) {
  g_lastDx = dx;
  g_lastDy = dy;
  lastJoystickMoveTime = millis();
}