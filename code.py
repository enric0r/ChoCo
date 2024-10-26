import time
import board
import keypad
import analogio
import digitalio
import usb_midi
import adafruit_midi
import displayio
import busio
import supervisor
import terminalio
import sys
import adafruit_displayio_ssd1306
from adafruit_display_text.label import Label
from adafruit_bitmap_font import bitmap_font
from adafruit_midi.note_on import NoteOn
from adafruit_midi.note_off import NoteOff

# Release any existing displays
displayio.release_displays()

# Initialize I2C bus and OLED display
i2c = busio.I2C(scl=board.GP15, sda=board.GP14)
display_bus = displayio.I2CDisplay(i2c, device_address=0x3C)
display = adafruit_displayio_ssd1306.SSD1306(display_bus, width=128, height=64)

# Initialize joystick analog inputs
x_axis = analogio.AnalogIn(board.A0)
y_axis = analogio.AnalogIn(board.A1)

# Initialize joystick button
joystick_button = digitalio.DigitalInOut(board.GP7)
joystick_button.direction = digitalio.Direction.INPUT
joystick_button.pull = digitalio.Pull.UP

# Mapping of the button positions (use None for unused positions)
key_map = [
    10, 11, 12, None,
     1, 3, 5, None,
    0, 2, 4, 6
]

# Load font
font = bitmap_font.load_font("fonts/SpaceGrotesk38.bdf")

# Create display elements
splash = displayio.Group()
display.root_group = splash

# Background rectangle
color_bitmap = displayio.Bitmap(128, 64, 1)
color_palette = displayio.Palette(1)
color_palette[0] = 0xFFFFFF  # White
bg_sprite = displayio.TileGrid(color_bitmap, pixel_shader=color_palette, x=0, y=0)
splash.append(bg_sprite)

# Inner black rectangle
inner_bitmap = displayio.Bitmap(126, 62, 1)
inner_palette = displayio.Palette(1)
inner_palette[0] = 0x000000  # Black
inner_sprite = displayio.TileGrid(inner_bitmap, pixel_shader=inner_palette, x=1, y=1)
splash.append(inner_sprite)

#Loading screen
banner = Label(terminalio.FONT, text="ChoCo\nLoading...", color=0xFFFFFF)
banner.anchor_point = (0.5, 0.5)
banner.anchored_position = (64, 32)
splash.append(banner)
time.sleep(2)
splash.remove(banner)

# Label for displaying chord
chord_label = Label(font, text="", color=0xFFFFFF)
chord_label.anchor_point = (0.5, 0.5)
chord_label.anchored_position = (64, 32)
splash.append(chord_label)

# Define the row and column pins for the keypad
row_pins = [board.GP0, board.GP1, board.GP2]
col_pins = [board.GP3, board.GP4, board.GP5, board.GP6]

# Initialize the keypad matrix
keys = keypad.KeyMatrix(row_pins, col_pins, columns_to_anodes=False)

# Initialize MIDI
midi = adafruit_midi.MIDI(midi_out=usb_midi.ports[1], out_channel=0)

# Define the MIDI note offsets for major and minor scales
scale_offsets = {
    "major": [0, 2, 4, 5, 7, 9, 11],
    "minor": [0, 2, 3, 5, 7, 8, 10],
    "harmonic_minor": [0, 2, 3, 5, 7, 8, 11],
    "melodic_minor": [0, 2, 3, 5, 7, 9, 11],
    "dorian": [0, 2, 3, 5, 7, 9, 10],
    "phrygian": [0, 1, 3, 5, 7, 8, 10],
    "lydian": [0, 2, 4, 6, 7, 9, 11],
    "mixolydian": [0, 2, 4, 5, 7, 9, 10],
    "locrian": [0, 1, 3, 5, 6, 8, 10],
}

# Define chord names based on intervals
chord_names = {
    (0, 4, 7): "Maj",
    (0, 3, 7): "Min",
    (0, 4, 8): "Aug",
    (0, 3, 6): "Dim",
    (0, 5, 7): "Sus4",
    (0, 2, 7): "Sus2",
    (0, 4, 7, 11): "Maj7",
    (0, 3, 7, 10): "Min7",
}

# Track the currently selected root note and scale type
root_note_index = 0
current_scale_type = "major"

# Track the currently playing chord
current_chord = None

# Define the grace period in seconds
grace_period = 0.3
last_joystick_move_time = 0

def get_analog_value(pin):
    return int((pin.value / 65535) * 127)

# Function to play a chord
def play_chord(chord):
    global current_chord
    for note in chord:
        midi.send(NoteOn(note, 127))  # Note on with velocity 127
    current_chord = chord
    update_displayed_chord(chord)

# Function to stop playing a chord
def stop_chord(chord):
    global current_chord
    for note in chord:
        midi.send(NoteOff(note, 0))  # Note off with velocity 0
    current_chord = None
    update_displayed_chord([])  # Clear display when chord stops
    
# Function to update the displayed chord and name
def update_displayed_chord(chord):
    global chord_label
    if chord:
        chord_name = get_chord_name(chord)
        chord_notes = ", ".join(map(str, chord))
        chord_label.text = f"{chord_name}"
        print(f"Currently playing chord: {chord_label.text}")
    else:
        chord_label.text = ""
        print("Chord stopped, display cleared")

def map_notes_to_scale(root_note, scale_type):
    scale_offset = scale_offsets[scale_type]
    return [(root_note + offset) % 12 + 60 for offset in scale_offset]

# Function to determine chord name based on intervals
def get_chord_name(chord):
    intervals = tuple((note - chord[0]) % 12 for note in chord)
    for key, name in chord_names.items():
        if intervals == key:
            return name
    return "Unknown"

def handle_joystick(x_value, y_value):
    global current_chord
    global last_joystick_move_time
    
    current_time = time.monotonic()
    if current_time - last_joystick_move_time < grace_period:
        return  # Skip processing if within the grace period

    if current_chord is None:
        return  # No chord to alter

    root_note = current_chord[0]
    current_chord_name = get_chord_name(current_chord)
    if "Min" in current_chord_name:
        if y_value > 100:  # UP: Switch to Min7
            stop_chord(current_chord)
            current_chord = generate_chord(root_note, "min7")
            play_chord(current_chord)
            print("Changed to Min7 chord")
            last_joystick_move_time = current_time
        elif y_value < 27:  # DOWN: Switch to Maj
            stop_chord(current_chord)
            current_chord = generate_chord(root_note, "major")
            play_chord(current_chord)
            print("Changed to Maj chord")
            last_joystick_move_time = current_time
    elif "Maj" in current_chord_name:
        if y_value > 100:  # UP: Switch to Maj7
            stop_chord(current_chord)
            current_chord = generate_chord(root_note, "maj7")
            play_chord(current_chord)
            print("Changed to Maj7 chord")
            last_joystick_move_time = current_time
        elif y_value < 27:  # DOWN: Switch to Min
            stop_chord(current_chord)
            current_chord = generate_chord(root_note, "minor")
            play_chord(current_chord)
            print("Changed to Min chord")
            last_joystick_move_time = current_time
    elif "min7" in current_chord_name or "maj7" in current_chord_name:
        if y_value < 27:  # DOWN: Switch to root chord
            stop_chord(current_chord)
            current_chord = generate_chord(root_note, "minor" if "min7" in current_chord_name else "major")
            play_chord(current_chord)
            print("Changed to root chord")
            last_joystick_move_time = current_time
            
    if x_value < 27:  # LEFT: Change chord to sus2
        stop_chord(current_chord)
        current_chord = generate_chord(root_note, "sus2")
        play_chord(current_chord)
        print("Changed to sus2 chord")
        last_joystick_move_time = current_time
    elif x_value > 100:  # RIGHT: Change chord to sus4
        stop_chord(current_chord)
        current_chord = generate_chord(root_note, "sus4")
        play_chord(current_chord)
        print("Changed to sus4 chord")
        last_joystick_move_time = current_time

# Function to generate a chord based on root note and chord type
def generate_chord(root_note, chord_type):
    chord_intervals = {
        'major': [0, 4, 7],
        'minor': [0, 3, 7],
        'diminished': [0, 3, 6],
        'augmented': [0, 4, 8],
        '7th': [0, 4, 7, 10],
        'maj7': [0, 4, 7, 11],
        'min7': [0, 3, 7, 10],
        'sus2': [0, 2, 7],
        'sus4': [0, 5, 7],
        'dominant7': [0, 4, 7, 10],
        
    }
    return [root_note + interval for interval in chord_intervals[chord_type]]

def handle_function_keys(key):
    global root_note_index, current_scale_type
    if key == 10:  # Function key to change root note
        root_note_index = (root_note_index + 1) % 12
        print(f"Root note changed to {root_note_index}")
    elif key == 11:  # Function key to change scale type
        scale_types = list(scale_offsets.keys())
        current_scale_index = scale_types.index(current_scale_type)
        current_scale_type = scale_types[(current_scale_index + 1) % len(scale_types)]
        print(f"Scale type changed to {current_scale_type}")
    elif key == 12:  # Additional function key for other settings
        supervisor.reload()

def get_chord_type_for_scale(scale_type, degree):
    """
    Given a scale type and a degree, return the chord type for that degree.
    """
    scale_chords = {
        'major': ['major', 'minor', 'minor', 'major', 'major', 'minor', 'diminished'],
        'minor': ['minor', 'diminished', 'major', 'minor', 'minor', 'major', 'major'],
        'harmonic_minor': ['minor', 'diminished', 'augmented', 'minor', 'major', 'major', 'diminished'],
        'melodic_minor': ['minor', 'minor', 'augmented', 'major', 'major', 'diminished', 'diminished'],
        'dorian': ['minor', 'minor', 'major', 'major', 'minor', 'diminished', 'major'],
        'phrygian': ['minor', 'major', 'major', 'minor', 'minor', 'major', 'diminished'],
        'lydian': ['major', 'major', 'minor', 'diminished', 'major', 'minor', 'minor'],
        'mixolydian': ['major', 'minor', 'diminished', 'major', 'minor', 'minor', 'major'],
        'locrian': ['diminished', 'major', 'minor', 'minor', 'major', 'major', 'minor'],
    }
    return scale_chords[scale_type][degree]

# Main loop
while True:
    # Handle keypad events
    event = keys.events.get()
    if event:
        row, col = divmod(event.key_number, len(col_pins))
        mapped_key = key_map[event.key_number]

        if event.pressed:
            if mapped_key is not None:
                if mapped_key in [10, 11, 12]:  # Function keys
                    handle_function_keys(mapped_key)
                else:  # Chord keys
                    scale_notes = map_notes_to_scale(root_note_index, current_scale_type)
                    root_note = scale_notes[mapped_key]
                    
                    chord_type = get_chord_type_for_scale(current_scale_type, mapped_key)
                    chord = generate_chord(root_note, chord_type)
                    
                    if chord:
                        if current_chord is not None:
                            stop_chord(current_chord)
                        current_chord = chord
                        play_chord(chord)
                        print(f"Chord {chord} pressed at row {row}, col {col}")
                        update_displayed_chord(current_chord)
        elif event.released:
            if mapped_key is not None:  # Chord keys released
                if current_chord:
                    stop_chord(current_chord)
                    current_chord = None
                    print(f"Chord released at row {row}, col {col}")
                    chord_label.text = ""

    # Handle joystick events
    x_value = get_analog_value(x_axis)
    y_value = get_analog_value(y_axis)
    
    handle_joystick(x_value, y_value)
  
