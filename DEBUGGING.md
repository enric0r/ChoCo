# Keypad Debugging Changes

## Changes Made

### 1. **Controls.cpp - Added Comprehensive Logging**
   - `scanKeypad()`: Now prints when a raw key is detected, showing row/col
   - `getKey()`: Extensive debug logging showing state transitions
   - `setupControls()`: Logs pin configuration and does immediate test scan

### 2. **main.cpp - Fixed Blocking Issues**
   - Added 2-second delay after Serial.begin() for monitor to open
   - Throttled `updateDisplay()` to once per 100ms (was every loop)
   - Added 10ms delay in loop to prevent excessive polling
   - Added "Main loop running..." message every 5 seconds

### 3. **Display.cpp - Reduced Splash Screen Time**
   - Reduced splash from 5 seconds to 2 seconds
   - Reduced frame interval from 2 seconds to 500ms
   - Added logging to show when splash starts/ends

### 4. **Created Test File: test/keypad_test.cpp**
   - Standalone hardware test
   - Direct matrix scanning without debounce logic
   - To use: temporarily replace contents of main.cpp with this file

## Diagnostic Steps

### Step 1: Upload and Monitor Serial Output
```powershell
pio run --target upload
pio device monitor
```

Look for these messages in order:
1. "ChoCo MIDI Controller Starting..."
2. "MIDI initialized"
3. "Setting up controls..." (with pin details)
4. "Testing immediate scan..."
5. "Controls setup complete"
6. "Display initialized"
7. "Setup complete - entering main loop"
8. "Main loop running..." (every 5 seconds)

### Step 2: Press a Key and Check For:
- **Raw scan level**: "Raw key detected: X at row=Y col=Z"
- **Debounce level**: "Key changed from NO_KEY to X"
- **Report level**: "Reporting key after debounce: X"
- **Main loop**: "Key pressed: X"

### Step 3: If No Messages When Pressing Keys

This means hardware issue or wiring problem. Try the standalone test:
1. Copy contents of `test/keypad_test.cpp` to `src/main.cpp`
2. Upload and monitor
3. This bypasses all the app logic and tests hardware directly

## Common Issues to Check

### Issue: No "Raw key detected" messages
**Problem**: Hardware not connected or wrong pins
**Check**: 
- Verify wiring matches pin definitions
- Row pins (GP0, GP1, GP2) should go to one side of switches
- Col pins (GP3, GP4, GP5, GP6) should go to other side
- No pull-down resistors needed (using INPUT_PULLUP)

### Issue: "Raw key detected" but no "Key changed"
**Problem**: Bug in getKey() logic
**Solution**: Already added extensive logging to diagnose

### Issue: "Key changed" but no "Reporting key"
**Problem**: Debounce delay not being met (key bouncing too much)
**Solution**: Try reducing debounceDelay from 50ms to 10ms in Controls.cpp

### Issue: Messages delayed by 5+ seconds
**Problem**: Splash screen or display blocking
**Solution**: Already reduced splash time and throttled display updates

## Quick Reference: Pin Mapping

```
Keypad Matrix Layout:
         Col0  Col1  Col2  Col3
         GP3   GP4   GP5   GP6
Row0 GP2  0     2     4    6
Row1 GP1  1     3     5    -
Row2 GP0  A     B     C    -

Key Functions:
0-6: Play chord degrees
A: Change root note
B: Cycle scale type
C: Modifier key (inversion edit mode when held)
```

## Next Steps

After uploading with these changes, monitor serial output and report:
1. What messages appear during startup
2. What happens when you press a key
3. Any error messages or missing expected outputs

This will tell us exactly where the problem is occurring in the chain.
